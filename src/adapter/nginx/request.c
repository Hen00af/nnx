#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "../../internal/nnx_internal.h"

extern ngx_module_t ngx_http_nnx_module;

typedef struct nnx_nginx_request_state {
    nnx_app *app;
    const nnx_adapter *adapter;
    nnx_ctx ctx;
    nnx_handler handler;
    int error_status;
    int auto_options;
    unsigned allowed_methods;
    ngx_msec_t started;
} nnx_nginx_request_state;

static int nnx_method_from_nginx(ngx_http_request_t *r, nnx_method *m)
{
    if (r->method == NGX_HTTP_GET) { *m = NNX_GET; return 0; }
    if (r->method == NGX_HTTP_POST) { *m = NNX_POST; return 0; }
    if (r->method == NGX_HTTP_PUT) { *m = NNX_PUT; return 0; }
    if (r->method == NGX_HTTP_PATCH) { *m = NNX_PATCH; return 0; }
    if (r->method == NGX_HTTP_DELETE) { *m = NNX_DELETE; return 0; }
    if (r->method == NGX_HTTP_HEAD) { *m = NNX_HEAD; return 0; }
    if (r->method == NGX_HTTP_OPTIONS) { *m = NNX_OPTIONS; return 0; }
    return -1;
}

static char *nnx_pool_cstr(ngx_pool_t *pool, const u_char *data, size_t len)
{
    char *out = ngx_pnalloc(pool, len + 1);
    if (!out) return NULL;
    if (len) ngx_memcpy(out, data, len);
    out[len] = 0;
    return out;
}

static ngx_int_t nnx_dispatch_request(ngx_http_request_t *r,
                                      nnx_nginx_request_state *state)
{
    nnx_ctx *ctx = &state->ctx;

    if (state->auto_options) {
        nnx_dispatch_options(state->app, ctx, state->allowed_methods);
    } else if (state->error_status) {
        if (state->allowed_methods)
            nnx_set_allow_header(ctx, state->allowed_methods);
        nnx_dispatch_error(state->app, ctx, state->error_status);
    } else {
        nnx_dispatch(state->app, ctx, state->handler);
    }

    if (!ctx->response_sent)
        nnx_dispatch_error(state->app, ctx, 500);

    ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0,
        "[NNX] %V %V %i %Mms", &r->method_name, &r->uri,
        ctx->response_sent ? ctx->response_status : 500,
        ngx_current_msec - state->started);

    return ctx->response_sent ? NGX_OK : NGX_HTTP_INTERNAL_SERVER_ERROR;
}

static ngx_int_t nnx_copy_request_body(ngx_http_request_t *r,
                                       nnx_nginx_request_state *state)
{
    ngx_chain_t *cl;
    ngx_buf_t *b;
    u_char *body;
    u_char *dst;
    size_t total = 0;
    size_t n;
    ssize_t read_n;

    if (!r->request_body || !r->request_body->bufs) {
        nnx_ctx_set_body(&state->ctx, NULL, 0);
        return NGX_OK;
    }

    for (cl = r->request_body->bufs; cl; cl = cl->next) {
        b = cl->buf;
        if (ngx_buf_in_memory(b))
            total += (size_t)(b->last - b->pos);
        else if (b->in_file)
            total += (size_t)(b->file_last - b->file_pos);
    }

    body = ngx_pnalloc(r->pool, total + 1);
    if (!body) return NGX_HTTP_INTERNAL_SERVER_ERROR;
    dst = body;

    for (cl = r->request_body->bufs; cl; cl = cl->next) {
        b = cl->buf;
        if (ngx_buf_in_memory(b)) {
            n = (size_t)(b->last - b->pos);
            if (n) {
                ngx_memcpy(dst, b->pos, n);
                dst += n;
            }
        } else if (b->in_file) {
            n = (size_t)(b->file_last - b->file_pos);
            if (n) {
                read_n = ngx_read_file(b->file, dst, n, b->file_pos);
                if (read_n == NGX_ERROR || (size_t)read_n != n)
                    return NGX_HTTP_INTERNAL_SERVER_ERROR;
                dst += n;
            }
        }
    }

    body[total] = 0;
    nnx_ctx_set_body(&state->ctx, body, total);
    return NGX_OK;
}

static void nnx_body_ready(ngx_http_request_t *r)
{
    nnx_nginx_request_state *state;
    ngx_int_t rc;

    state = ngx_http_get_module_ctx(r, ngx_http_nnx_module);
    if (!state) {
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    rc = nnx_copy_request_body(r, state);
    if (rc == NGX_OK) {
        rc = nnx_dispatch_request(r, state);
    } else {
        state->error_status = 500;
        rc = nnx_dispatch_request(r, state);
    }
    ngx_http_finalize_request(r, rc);
}

ngx_int_t nnx_nginx_handle_request(ngx_http_request_t *r, nnx_app *app,
                                   const nnx_adapter *adapter)
{
    nnx_nginx_request_state *state;
    nnx_method method;
    char *path;
    char *method_name;
    ngx_int_t rc;

    if (!app || !adapter) return NGX_HTTP_INTERNAL_SERVER_ERROR;

    state = ngx_pcalloc(r->pool, sizeof(*state));
    if (!state) return NGX_HTTP_INTERNAL_SERVER_ERROR;

    path = nnx_pool_cstr(r->pool, r->uri.data, r->uri.len);
    method_name = nnx_pool_cstr(r->pool, r->method_name.data, r->method_name.len);
    if (!path || !method_name) return NGX_HTTP_INTERNAL_SERVER_ERROR;

    state->app = app;
    state->adapter = adapter;
    state->started = ngx_current_msec;
    nnx_ctx_init(&state->ctx, r, adapter, method_name, path);

    if (nnx_method_from_nginx(r, &method) != 0) {
        state->allowed_methods = nnx_route_allowed_methods(app, path, &state->ctx);
        state->error_status = state->allowed_methods ? 405 : 404;
        return nnx_dispatch_request(r, state);
    }

    state->handler = nnx_match_route(app, method, path, &state->ctx);
    if (!state->handler && method == NNX_HEAD)
        state->handler = nnx_match_route(app, NNX_GET, path, &state->ctx);

    if (!state->handler) {
        state->allowed_methods = nnx_route_allowed_methods(app, path, &state->ctx);
        if (!state->allowed_methods) {
            state->error_status = 404;
        } else if (method == NNX_OPTIONS) {
            state->auto_options = 1;
        } else {
            state->error_status = 405;
        }
        return nnx_dispatch_request(r, state);
    }

    if (r->headers_in.content_length_n > 0 || r->headers_in.chunked) {
        ngx_http_set_ctx(r, state, ngx_http_nnx_module);
        r->request_body_in_single_buf = 1;
        rc = ngx_http_read_client_request_body(r, nnx_body_ready);
        if (rc >= NGX_HTTP_SPECIAL_RESPONSE)
            return rc;
        return NGX_DONE;
    }

    return nnx_dispatch_request(r, state);
}

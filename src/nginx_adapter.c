#include <string.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "nnx_internal.h"

static nnx_app *nnx_active_app;

static ngx_int_t nnx_init_process(ngx_cycle_t *cycle)
{
    (void)cycle;
    nnx_active_app = nnx_new();
    if (!nnx_active_app) return NGX_ERROR;
    if (nnx_register(nnx_active_app) != 0) {
        nnx_free(nnx_active_app);
        nnx_active_app = NULL;
        return NGX_ERROR;
    }
    return NGX_OK;
}

static void nnx_exit_process(ngx_cycle_t *cycle)
{
    (void)cycle;
    nnx_free(nnx_active_app);
    nnx_active_app = NULL;
}

static int nnx_method_from_nginx(ngx_http_request_t *r, nnx_method *m)
{
    if (r->method == NGX_HTTP_GET) { *m = NNX_GET; return 0; }
    if (r->method == NGX_HTTP_POST) { *m = NNX_POST; return 0; }
    return -1;
}

int nnx_adapter_send(nnx_ctx *ctx, int status, const char *body)
{
    ngx_http_request_t *r; ngx_buf_t *b; ngx_chain_t out; u_char *data; size_t len; ngx_int_t rc;
    if (!ctx || !ctx->native_request || !body) return -1;
    r = ctx->native_request; len = strlen(body);
    r->headers_out.status = status;
    r->headers_out.content_length_n = (off_t)len;
    ngx_str_set(&r->headers_out.content_type, "text/plain");
    rc = ngx_http_send_header(r);
    if (rc == NGX_ERROR || rc > NGX_OK) return -1;
    if (r->header_only) return 0;
    data = ngx_pnalloc(r->pool, len);
    if (!data && len != 0) return -1;
    if (len != 0) ngx_memcpy(data, body, len);
    b = ngx_calloc_buf(r->pool);
    if (!b) return -1;
    b->pos = data; b->last = data + len;
    b->memory = 1; b->last_buf = 1;
    out.buf = b; out.next = NULL;
    rc = ngx_http_output_filter(r, &out);
    return rc == NGX_ERROR ? -1 : 0;
}

static ngx_int_t nnx_http_handler(ngx_http_request_t *r)
{
    nnx_method method; nnx_handler handler; nnx_ctx ctx; char *path; ngx_msec_t started;
    started = ngx_current_msec;
    if (!nnx_active_app) return NGX_HTTP_INTERNAL_SERVER_ERROR;
    if (nnx_method_from_nginx(r, &method) != 0) return NGX_HTTP_NOT_ALLOWED;
    path = ngx_pnalloc(r->pool, r->uri.len + 1);
    if (!path) return NGX_HTTP_INTERNAL_SERVER_ERROR;
    ngx_memcpy(path, r->uri.data, r->uri.len); path[r->uri.len] = '\0';
    handler = nnx_match_route(nnx_active_app, method, path);
    if (!handler) return NGX_HTTP_NOT_FOUND;
    ctx.native_request = r; ctx.response_sent = 0; ctx.response_status = 0;
    handler(&ctx);
    ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0,
        "[NNX] %V %V %i %Mms",
        &r->method_name, &r->uri,
        ctx.response_sent ? ctx.response_status : 500,
        ngx_current_msec - started);
    return ctx.response_sent ? NGX_OK : NGX_HTTP_INTERNAL_SERVER_ERROR;
}

static char *nnx_enable(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t *clcf; (void)cmd; (void)conf;
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = nnx_http_handler;
    return NGX_CONF_OK;
}

static ngx_command_t nnx_commands[] = {
    { ngx_string("nnx"), NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS, nnx_enable, 0, 0, NULL },
    ngx_null_command
};
static ngx_http_module_t nnx_module_ctx = { NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL };
ngx_module_t ngx_http_nnx_module = {
    NGX_MODULE_V1, &nnx_module_ctx, nnx_commands, NGX_HTTP_MODULE,
    NULL,NULL,nnx_init_process,NULL,NULL,nnx_exit_process,NULL,
    NGX_MODULE_V1_PADDING
};

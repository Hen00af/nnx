#include "../internal/nnx_internal.h"

void nnx_ctx_init(nnx_ctx *ctx, void *request, const nnx_adapter *adapter,
                  const char *method_name, const char *path)
{
    ctx->adapter_request = request;
    ctx->adapter = adapter;
    ctx->response_sent = 0;
    ctx->response_status = 0;
    ctx->method_name = method_name;
    ctx->path = path;
    ctx->body = NULL;
    ctx->body_len = 0;
    ctx->param_count = 0;
    ctx->wildcard[0] = 0;
    ctx->app = NULL;
    ctx->endpoint = NULL;
    ctx->middleware_index = 0;
    ctx->request_id[0] = 0;
}

void nnx_ctx_set_body(nnx_ctx *ctx, const void *body, size_t len)
{
    if (!ctx) return;
    ctx->body = body;
    ctx->body_len = len;
}

const char *nnx_method_name(nnx_ctx *ctx)
{ return ctx ? ctx->method_name : NULL; }

const char *nnx_path(nnx_ctx *ctx)
{ return ctx ? ctx->path : NULL; }

const char *nnx_query(nnx_ctx *ctx, const char *name)
{
    if (!ctx || !name || !ctx->adapter || !ctx->adapter->query) return NULL;
    return ctx->adapter->query(ctx->adapter_request, name);
}

const char *nnx_header(nnx_ctx *ctx, const char *name)
{
    if (!ctx || !name || !ctx->adapter || !ctx->adapter->header) return NULL;
    return ctx->adapter->header(ctx->adapter_request, name);
}

const char *nnx_request_id(nnx_ctx *ctx)
{ return ctx && ctx->request_id[0] ? ctx->request_id : NULL; }

const void *nnx_body(nnx_ctx *ctx, size_t *len)
{
    if (len) *len = ctx ? ctx->body_len : 0;
    return ctx ? ctx->body : NULL;
}

const char *nnx_body_text(nnx_ctx *ctx)
{
    static const char empty[] = "";
    return ctx && ctx->body ? (const char *)ctx->body : empty;
}

int nnx_status(nnx_ctx *ctx)
{ return ctx ? ctx->response_status : 0; }

int nnx_log(nnx_ctx *ctx, const char *message)
{
    if (!ctx || !message || !ctx->adapter || !ctx->adapter->log) return -1;
    return ctx->adapter->log(ctx->adapter_request, message);
}

void nnx_next(nnx_ctx *ctx)
{
    nnx_middleware_entry *entry;
    if (!ctx || !ctx->app) return;
    if (ctx->middleware_index < ctx->app->middleware_count) {
        entry = &ctx->app->middleware[ctx->middleware_index++];
        entry->fn(ctx, entry->data);
        return;
    }
    if (ctx->endpoint) {
        nnx_handler endpoint = ctx->endpoint;
        ctx->endpoint = NULL;
        endpoint(ctx);
    }
}

void nnx_dispatch(nnx_app *app, nnx_ctx *ctx, nnx_handler endpoint)
{
    if (!app || !ctx || !endpoint) return;
    ctx->app = app;
    ctx->endpoint = endpoint;
    ctx->middleware_index = 0;
    nnx_next(ctx);
}

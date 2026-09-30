#include "../internal/nnx_internal.h"

void nnx_ctx_init(nnx_ctx *ctx, void *request, const nnx_adapter *adapter)
{
    ctx->adapter_request = request;
    ctx->adapter = adapter;
    ctx->response_sent = 0;
    ctx->response_status = 0;
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

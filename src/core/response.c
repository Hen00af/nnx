#include <string.h>
#include "../internal/nnx_internal.h"

int nnx_set_header(nnx_ctx *ctx, const char *name, const char *value)
{
    if (!ctx || !name || !value || ctx->response_sent ||
        !ctx->adapter || !ctx->adapter->set_header)
        return -1;
    return ctx->adapter->set_header(ctx->adapter_request, name, value);
}

int nnx_blob(nnx_ctx *ctx, int status, const char *content_type,
             const void *data, size_t len)
{
    static const char empty = 0;
    if (!ctx || !content_type || (!data && len != 0) || ctx->response_sent ||
        !ctx->adapter || !ctx->adapter->send)
        return -1;
    if (!data) data = &empty;
    if (ctx->adapter->send(ctx->adapter_request, status, content_type, data, len) != 0)
        return -1;
    ctx->response_sent = 1;
    ctx->response_status = status;
    return 0;
}

int nnx_text(nnx_ctx *ctx, int status, const char *body)
{
    if (!body) return -1;
    return nnx_blob(ctx, status, "text/plain; charset=utf-8", body, strlen(body));
}

int nnx_send(nnx_ctx *ctx, int status, const char *body)
{ return nnx_text(ctx, status, body); }

int nnx_json(nnx_ctx *ctx, int status, const char *json)
{
    if (!json) return -1;
    return nnx_blob(ctx, status, "application/json; charset=utf-8", json, strlen(json));
}

int nnx_html(nnx_ctx *ctx, int status, const char *html)
{
    if (!html) return -1;
    return nnx_blob(ctx, status, "text/html; charset=utf-8", html, strlen(html));
}

int nnx_redirect(nnx_ctx *ctx, int status, const char *location)
{
    if (!location || nnx_set_header(ctx, "Location", location) != 0) return -1;
    return nnx_text(ctx, status, "");
}

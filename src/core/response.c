#include "../internal/nnx_internal.h"

int nnx_send(nnx_ctx *ctx, int status, const char *body)
{
    if (!ctx || !body || ctx->response_sent || !ctx->adapter || !ctx->adapter->send)
        return -1;
    if (ctx->adapter->send(ctx->adapter_request, status, body) != 0)
        return -1;
    ctx->response_sent = 1;
    ctx->response_status = status;
    return 0;
}

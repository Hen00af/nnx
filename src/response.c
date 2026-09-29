#include "nnx_internal.h"
int nnx_send(nnx_ctx *ctx, int status, const char *body)
{
    if (!ctx || !body || ctx->response_sent) return -1;
    if (nnx_adapter_send(ctx, status, body) != 0) return -1;
    ctx->response_sent = 1;
    ctx->response_status = status;
    return 0;
}

#include "../internal/nnx_internal.h"

void nnx_ctx_init(nnx_ctx *ctx, void *request, const nnx_adapter *adapter)
{
    ctx->adapter_request = request;
    ctx->adapter = adapter;
    ctx->response_sent = 0;
    ctx->response_status = 0;
}

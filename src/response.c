#include "nnx_internal.h"

static int nnx_send_type(nnx_ctx *ctx, int status, const char *body, const char *type)
{
    if (!ctx || !body || ctx->response_sent) return -1;
    if (nnx_adapter_send(ctx, status, body, type) != 0) return -1;
    ctx->response_sent = 1;
    ctx->response_status = status;
    return 0;
}

int nnx_send(nnx_ctx *ctx, int status, const char *body)
{ return nnx_text(ctx, status, body); }

int nnx_text(nnx_ctx *ctx, int status, const char *body)
{ return nnx_send_type(ctx, status, body, "text/plain; charset=utf-8"); }

int nnx_json(nnx_ctx *ctx, int status, const char *json)
{ return nnx_send_type(ctx, status, json, "application/json; charset=utf-8"); }

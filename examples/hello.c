#include <nnx.h>

static void hello(nnx_ctx *ctx)
{
    nnx_text(ctx, 200, "Hello, nnx!\n");
}

static void echo_body(nnx_ctx *ctx)
{
    nnx_text(ctx, 200, nnx_body_text(ctx));
}

int nnx_register(nnx_app *app)
{
    if (nnx_get(app, "/", hello) != 0) return -1;
    if (nnx_post(app, "/echo", echo_body) != 0) return -1;
    return 0;
}

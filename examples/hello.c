#include <nnx.h>

static void hello(nnx_ctx *ctx)
{
    nnx_send(ctx, 200, "Hello, nnx!\n");
}

int nnx_register(nnx_app *app)
{
    return nnx_get(app, "/", hello);
}

#include <nnx.h>

static void hello(nnx_ctx *ctx)
{
    nnx_send(ctx, 200, "Hello, nnx!");
}

int main(void)
{
    nnx_app *app = nnx_new();

    if (!app)
        return 1;
    if (nnx_get(app, "/", hello) != 0)
        return 1;
    return nnx_run(app, 8080);
}

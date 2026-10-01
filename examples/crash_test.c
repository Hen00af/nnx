#include <signal.h>
#include <nnx.h>

static void health(nnx_ctx *ctx)
{
    nnx_text(ctx, 200, "ok");
}

static void crash(nnx_ctx *ctx)
{
    (void)ctx;
    raise(SIGSEGV);
}

int nnx_register(nnx_app *app)
{
    if (nnx_get(app, "/health", health) != 0)
        return -1;
    if (nnx_get(app, "/crash", crash) != 0)
        return -1;
    return 0;
}

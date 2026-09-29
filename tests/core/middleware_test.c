#include <assert.h>
#include "../../src/internal/nnx_internal.h"

static int trace[8];
static int n;

static void endpoint(nnx_ctx *ctx) { (void)ctx; trace[n++] = 3; }
static void first(nnx_ctx *ctx, void *data)
{
    (void)data; trace[n++] = 1; nnx_next(ctx); trace[n++] = 5;
}
static void second(nnx_ctx *ctx, void *data)
{
    (void)data; trace[n++] = 2; nnx_next(ctx); trace[n++] = 4;
}

int main(void)
{
    nnx_app *app = nnx_new(); nnx_ctx ctx = {0};
    assert(app);
    assert(nnx_use(app, first, NULL) == 0);
    assert(nnx_use(app, second, NULL) == 0);
    nnx_dispatch(app, &ctx, endpoint);
    assert(n == 5);
    assert(trace[0] == 1 && trace[1] == 2 && trace[2] == 3);
    assert(trace[3] == 4 && trace[4] == 5);
    nnx_free(app);
    return 0;
}

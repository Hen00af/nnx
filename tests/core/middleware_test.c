#include <assert.h>
#include "../../src/internal/nnx_internal.h"

static int trace[16];
static int n;

static void endpoint(nnx_ctx *ctx) { (void)ctx; trace[n++] = 3; }

static void global_middleware(nnx_ctx *ctx, void *data)
{
    (void)data;
    trace[n++] = 1;
    nnx_next(ctx);
    trace[n++] = 5;
}

static void group_middleware(nnx_ctx *ctx, void *data)
{
    (void)data;
    trace[n++] = 2;
    nnx_next(ctx);
    trace[n++] = 4;
}

static void test_global_chain(void)
{
    nnx_app *app = nnx_new();
    nnx_ctx ctx = {0};

    n = 0;
    assert(app);
    assert(nnx_use(app, (nnx_middleware){global_middleware, NULL, NULL}) == 0);
    nnx_dispatch(app, &ctx, endpoint);

    assert(n == 3);
    assert(trace[0] == 1 && trace[1] == 3 && trace[2] == 5);
    nnx_free(app);
}

static void test_group_chain(void)
{
    nnx_app *app = nnx_new();
    nnx_group *api;
    nnx_ctx ctx = {0};
    nnx_handler handler;

    n = 0;
    assert(app);
    api = nnx_group_new(app, "/api");
    assert(api);

    assert(nnx_use(app, (nnx_middleware){global_middleware, NULL, NULL}) == 0);
    assert(nnx_group_use(api, (nnx_middleware){group_middleware, NULL, NULL}) == 0);
    assert(nnx_group_get(api, "/hello", endpoint) == 0);

    handler = nnx_match_route(app, NNX_GET, "/api/hello", &ctx);
    assert(handler == endpoint);
    assert(ctx.group == api);

    nnx_dispatch(app, &ctx, handler);

    assert(n == 5);
    assert(trace[0] == 1);
    assert(trace[1] == 2);
    assert(trace[2] == 3);
    assert(trace[3] == 4);
    assert(trace[4] == 5);

    nnx_free(app);
}

int main(void)
{
    test_global_chain();
    test_group_chain();
    return 0;
}

#include <assert.h>
#include "../../src/internal/nnx_internal.h"

static int trace[16];
static int n;

typedef struct marks {
    int before;
    int after;
} marks;

static void endpoint(nnx_ctx *ctx)
{
    (void)ctx;
    trace[n++] = 4;
}

static void around(nnx_ctx *ctx, void *data)
{
    marks *m = data;
    trace[n++] = m->before;
    nnx_next(ctx);
    trace[n++] = m->after;
}

static void test_nested_group_chain(void)
{
    nnx_app *app = nnx_new();
    nnx_group *api;
    nnx_group *v1;
    nnx_ctx ctx = {0};
    nnx_handler handler;
    marks global_marks = {1, 7};
    marks api_marks = {2, 6};
    marks v1_marks = {3, 5};

    n = 0;
    assert(app);
    api = nnx_group_new(app, "/api");
    assert(api);
    v1 = nnx_group_group(api, "/v1");
    assert(v1);

    assert(nnx_use(app, (nnx_middleware){around, &global_marks, NULL}) == 0);
    assert(nnx_group_use(api, (nnx_middleware){around, &api_marks, NULL}) == 0);
    assert(nnx_group_use(v1, (nnx_middleware){around, &v1_marks, NULL}) == 0);
    assert(nnx_group_get(v1, "/hello", endpoint) == 0);

    handler = nnx_match_route(app, NNX_GET, "/api/v1/hello", &ctx);
    assert(handler == endpoint);
    assert(ctx.group == v1);

    nnx_dispatch(app, &ctx, handler);

    assert(n == 7);
    assert(trace[0] == 1);
    assert(trace[1] == 2);
    assert(trace[2] == 3);
    assert(trace[3] == 4);
    assert(trace[4] == 5);
    assert(trace[5] == 6);
    assert(trace[6] == 7);

    nnx_free(app);
}

int main(void)
{
    test_nested_group_chain();
    return 0;
}

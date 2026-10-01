#include <assert.h>
#include "../../src/internal/nnx_internal.h"

static int trace[16];
static int n;
static int sent_status;

static int fake_send(void *request, int status, const char *type,
                     const void *body, size_t len)
{
    (void)request;
    (void)type;
    (void)body;
    (void)len;
    sent_status = status;
    return 0;
}

static const nnx_adapter fake = {
    fake_send, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

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


static void stop_middleware(nnx_ctx *ctx, void *data)
{
    (void)ctx;
    (void)data;
    trace[n++] = 2;
}

static void double_next_middleware(nnx_ctx *ctx, void *data)
{
    (void)data;
    trace[n++] = 1;
    nnx_next(ctx);
    nnx_next(ctx);
    trace[n++] = 3;
}

static void should_not_run(nnx_ctx *ctx)
{
    (void)ctx;
    trace[n++] = 9;
}

static void test_double_next_is_guarded(void)
{
    nnx_app *app = nnx_new();
    nnx_ctx ctx = {0};

    n = 0;
    assert(app);
    assert(nnx_use(app, (nnx_middleware){double_next_middleware, NULL, NULL}) == 0);
    assert(nnx_use(app, (nnx_middleware){stop_middleware, NULL, NULL}) == 0);

    nnx_dispatch(app, &ctx, should_not_run);

    assert(n == 3);
    assert(trace[0] == 1);
    assert(trace[1] == 2);
    assert(trace[2] == 3);

    nnx_free(app);
}


static void respond_then_next(nnx_ctx *ctx, void *data)
{
    (void)data;
    trace[n++] = 1;
    assert(nnx_text(ctx, 204, "") == 0);
    nnx_next(ctx);
    trace[n++] = 2;
}

static void test_response_short_circuits_next(void)
{
    nnx_app *app = nnx_new();
    nnx_ctx ctx;

    n = 0;
    sent_status = 0;
    assert(app);
    assert(nnx_use(app, (nnx_middleware){respond_then_next, NULL, NULL}) == 0);

    nnx_ctx_init(&ctx, NULL, &fake, "GET", "/short");
    nnx_dispatch(app, &ctx, should_not_run);

    assert(sent_status == 204);
    assert(n == 2);
    assert(trace[0] == 1);
    assert(trace[1] == 2);

    nnx_free(app);
}

int main(void)
{
    test_nested_group_chain();
    test_double_next_is_guarded();
    test_response_short_circuits_next();
    return 0;
}

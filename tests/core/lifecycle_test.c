#include <assert.h>
#include <stdlib.h>
#include <nnx.h>

static int destroyed;

static void noop_handler(nnx_ctx *ctx)
{
    (void)ctx;
}

static void noop_middleware(nnx_ctx *ctx, void *data)
{
    (void)ctx;
    (void)data;
}

static void destroy_counter(void *data)
{
    int *value = data;
    if (value) {
        destroyed += *value;
        free(value);
    }
}

static nnx_middleware owned_middleware(void)
{
    nnx_middleware m;
    int *value = malloc(sizeof(*value));

    assert(value);
    *value = 1;
    m.fn = noop_middleware;
    m.data = value;
    m.destroy = destroy_counter;
    return m;
}

int main(void)
{
    int i;

    destroyed = 0;
    for (i = 0; i < 1000; ++i) {
        nnx_app *app = nnx_new();
        nnx_group *api;
        nnx_group *v1;

        assert(app);
        assert(nnx_use(app, owned_middleware()) == 0);
        assert(nnx_get(app, "/", noop_handler) == 0);
        assert(nnx_post(app, "/echo", noop_handler) == 0);

        api = nnx_group_new(app, "/api");
        assert(api);
        assert(nnx_group_use(api, owned_middleware()) == 0);

        v1 = nnx_group_group(api, "/v1");
        assert(v1);
        assert(nnx_group_get(v1, "/users/:id", noop_handler) == 0);

        nnx_free(app);
    }

    assert(destroyed == 2000);
    return 0;
}

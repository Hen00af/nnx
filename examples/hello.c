#include <stdio.h>
#include <nnx.h>

static void request_start(nnx_ctx *ctx)
{
    (void)ctx;
}

static void hello(nnx_ctx *ctx)
{
    nnx_text(ctx, 200, "Hello, nnx!\n");
}

static void user(nnx_ctx *ctx)
{
    const char *id = nnx_param(ctx, "id");
    char json[256];
    snprintf(json, sizeof(json), "{\"id\":\"%s\"}\n", id ? id : "");
    nnx_json(ctx, 200, json);
}

static void search(nnx_ctx *ctx)
{
    const char *q = nnx_query(ctx, "q");
    nnx_text(ctx, 200, q ? q : "");
}

int nnx_register(nnx_app *app)
{
    if (nnx_use(app, request_start, NULL) != 0) return -1;
    if (nnx_get(app, "/", hello) != 0) return -1;
    if (nnx_get(app, "/users/:id", user) != 0) return -1;
    if (nnx_get(app, "/search", search) != 0) return -1;
    return 0;
}

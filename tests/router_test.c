#include <assert.h>
#include <string.h>
#include <nnx.h>
#include "../src/nnx_internal.h"

static void handler(nnx_ctx *ctx) { (void)ctx; }
static const char *nnx_adapter_query(nnx_ctx *ctx, const char *name) { (void)ctx; (void)name; return 0; }
static const char *nnx_adapter_header(nnx_ctx *ctx, const char *name) { (void)ctx; (void)name; return 0; }

int main(void)
{
    nnx_app *app = nnx_new();
    nnx_ctx ctx = {0};
    assert(app);
    assert(nnx_get(app, "/hello", handler) == 0);
    assert(nnx_get(app, "/users/:id", handler) == 0);
    assert(nnx_match_route(app, NNX_GET, "/hello", &ctx) == handler);
    assert(nnx_match_route(app, NNX_POST, "/hello", &ctx) == 0);
    assert(nnx_match_route(app, NNX_GET, "/users/42", &ctx) == handler);
    assert(strcmp(nnx_param(&ctx, "id"), "42") == 0);
    nnx_free(app);
    return 0;
}

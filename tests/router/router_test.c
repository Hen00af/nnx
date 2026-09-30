#include <assert.h>
#include <nnx.h>
#include "../../src/internal/nnx_internal.h"

static void handler(nnx_ctx *ctx) { (void)ctx; }

int main(void)
{
    nnx_app *app = nnx_new();
    assert(app);
    assert(nnx_get(app, "/hello", handler) == 0);
    assert(nnx_match_route(app, NNX_GET, "/hello") == handler);
    assert(nnx_match_route(app, NNX_POST, "/hello") == 0);
    assert(nnx_match_route(app, NNX_GET, "/missing") == 0);
    nnx_free(app);
    return 0;
}

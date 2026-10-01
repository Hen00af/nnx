#include <assert.h>
#include <string.h>
#include <nnx.h>
#include "../../src/internal/nnx_internal.h"

static void static_h(nnx_ctx *c) { (void)c; }
static void param_h(nnx_ctx *c) { (void)c; }
static void wildcard_h(nnx_ctx *c) { (void)c; }
static void any_h(nnx_ctx *c) { (void)c; }
static void group_h(nnx_ctx *c) { (void)c; }
static void head_h(nnx_ctx *c) { (void)c; }

int main(void)
{
    nnx_app *app = nnx_new();
    nnx_group *api;
    nnx_group *v1;
    nnx_ctx ctx = {0};
    char allow[128];

    assert(app);
    assert(nnx_get(app, "/users/:id", param_h) == 0);
    assert(nnx_get(app, "/users/me", static_h) == 0);
    assert(nnx_get(app, "/assets/*", wildcard_h) == 0);
    assert(nnx_any(app, "/health", any_h) == 0);
    assert(nnx_put(app, "/users/:id", param_h) == 0);
    assert(nnx_get(app, "/head-fallback", static_h) == 0);
    assert(nnx_get(app, "/head-explicit", static_h) == 0);
    assert(nnx_head(app, "/head-explicit", head_h) == 0);

    assert(nnx_get(app, "/users/:id", param_h) == -1);
    assert(nnx_get(app, "/health", static_h) == -1);
    assert(nnx_any(app, "/users/me", any_h) == -1);

    api = nnx_group_new(app, "/api");
    assert(api);
    assert(nnx_group_get(api, "/users/:id", group_h) == 0);
    v1 = nnx_group_group(api, "/v1");
    assert(v1);
    assert(nnx_group_get(v1, "/posts/:id", group_h) == 0);
    assert(nnx_group_get(v1, "/posts/:id", group_h) == -1);

    assert(nnx_match_route(app, NNX_GET, "/users/me", &ctx) == static_h);
    assert(nnx_match_route(app, NNX_GET, "/users/42", &ctx) == param_h);
    assert(strcmp(nnx_param(&ctx, "id"), "42") == 0);

    assert(nnx_match_route(app, NNX_GET, "/assets/css/app.css", &ctx) == wildcard_h);
    assert(strcmp(nnx_wildcard(&ctx), "css/app.css") == 0);

    assert(nnx_match_route(app, NNX_DELETE, "/health", &ctx) == any_h);
    assert(nnx_match_route(app, NNX_HEAD, "/head-fallback", &ctx) == static_h);
    assert(nnx_match_route(app, NNX_HEAD, "/head-explicit", &ctx) == head_h);
    assert(nnx_match_route(app, NNX_PUT, "/users/7", &ctx) == param_h);
    assert(nnx_match_route(app, NNX_POST, "/missing", &ctx) == 0);
    assert(nnx_route_path_exists(app, "/users/123") == 1);
    assert(nnx_route_path_exists(app, "/assets/js/app.js") == 1);
    assert(nnx_route_path_exists(app, "/definitely-missing") == 0);
    assert(nnx_route_allow(app, "/users/123", allow, sizeof(allow)) == 0);
    assert(strcmp(allow, "GET, HEAD, PUT") == 0);
    assert(nnx_route_allow(app, "/definitely-missing", allow, sizeof(allow)) == -1);

    assert(nnx_match_route(app, NNX_GET, "/api/users/99", &ctx) == group_h);
    assert(ctx.group == api);
    assert(strcmp(nnx_param(&ctx, "id"), "99") == 0);

    assert(nnx_match_route(app, NNX_GET, "/api/v1/posts/7", &ctx) == group_h);
    assert(ctx.group == v1);
    assert(strcmp(nnx_param(&ctx, "id"), "7") == 0);

    nnx_free(app);
    return 0;
}

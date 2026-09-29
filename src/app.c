#include <stdlib.h>
#include "nnx_internal.h"

nnx_app *nnx_new(void) { return calloc(1, sizeof(nnx_app)); }

void nnx_free(nnx_app *app)
{
    size_t i;
    if (!app) return;
    for (i = 0; i < app->route_count; ++i) free(app->routes[i].path);
    free(app->routes);
    free(app->middleware);
    free(app);
}

int nnx_get(nnx_app *app, const char *path, nnx_handler h)
{ return nnx_add_route(app, NNX_GET, path, h); }

int nnx_post(nnx_app *app, const char *path, nnx_handler h)
{ return nnx_add_route(app, NNX_POST, path, h); }

int nnx_use(nnx_app *app, nnx_middleware before, nnx_middleware after)
{
    nnx_middleware_pair *p;
    size_t cap;
    if (!app || (!before && !after)) return -1;
    if (app->middleware_count == app->middleware_capacity) {
        cap = app->middleware_capacity ? app->middleware_capacity * 2 : 4;
        p = realloc(app->middleware, cap * sizeof(*p));
        if (!p) return -1;
        app->middleware = p;
        app->middleware_capacity = cap;
    }
    app->middleware[app->middleware_count].before = before;
    app->middleware[app->middleware_count].after = after;
    ++app->middleware_count;
    return 0;
}

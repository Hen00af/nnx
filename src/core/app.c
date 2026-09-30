#include <stdlib.h>
#include "../internal/nnx_internal.h"

nnx_app *nnx_new(void) { return calloc(1, sizeof(nnx_app)); }

void nnx_free(nnx_app *app)
{
    size_t i;
    if (!app) return;
    for (i = 0; i < app->route_count; ++i) free(app->routes[i].path);
    for (i = 0; i < app->middleware_count; ++i)
        if (app->middleware[i].destroy)
            app->middleware[i].destroy(app->middleware[i].data);
    free(app->routes);
    free(app->middleware);
    free(app);
}

#define NNX_ROUTE_FN(name, method) \
int name(nnx_app *app, const char *path, nnx_handler h) \
{ return nnx_add_route(app, method, path, h); }

NNX_ROUTE_FN(nnx_get, NNX_GET)
NNX_ROUTE_FN(nnx_post, NNX_POST)
NNX_ROUTE_FN(nnx_put, NNX_PUT)
NNX_ROUTE_FN(nnx_patch, NNX_PATCH)
NNX_ROUTE_FN(nnx_delete, NNX_DELETE)
NNX_ROUTE_FN(nnx_head, NNX_HEAD)
NNX_ROUTE_FN(nnx_options, NNX_OPTIONS)
NNX_ROUTE_FN(nnx_any, NNX_METHOD_ANY)

int nnx_use(nnx_app *app, nnx_middleware middleware)
{
    nnx_middleware_entry *entries;
    size_t cap;

    if (!app || !middleware.fn) return -1;
    if (app->middleware_count == app->middleware_capacity) {
        cap = app->middleware_capacity ? app->middleware_capacity * 2 : 4;
        entries = realloc(app->middleware, cap * sizeof(*entries));
        if (!entries) {
            if (middleware.destroy) middleware.destroy(middleware.data);
            return -1;
        }
        app->middleware = entries;
        app->middleware_capacity = cap;
    }
    app->middleware[app->middleware_count].fn = middleware.fn;
    app->middleware[app->middleware_count].data = middleware.data;
    app->middleware[app->middleware_count].destroy = middleware.destroy;
    ++app->middleware_count;
    return 0;
}

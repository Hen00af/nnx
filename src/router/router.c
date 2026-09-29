#include <stdlib.h>
#include <string.h>
#include "../internal/nnx_internal.h"

static char *nnx_strdup(const char *s)
{
    size_t n; char *p;
    if (!s) return NULL;
    n = strlen(s) + 1;
    p = malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

int nnx_add_route(nnx_app *app, nnx_method method, const char *path, nnx_handler h)
{
    nnx_route *routes; size_t cap;
    if (!app || !path || path[0] != '/' || !h) return -1;
    if (app->route_count == app->route_capacity) {
        cap = app->route_capacity ? app->route_capacity * 2 : 8;
        routes = realloc(app->routes, cap * sizeof(*routes));
        if (!routes) return -1;
        app->routes = routes; app->route_capacity = cap;
    }
    app->routes[app->route_count].path = nnx_strdup(path);
    if (!app->routes[app->route_count].path) return -1;
    app->routes[app->route_count].method = method;
    app->routes[app->route_count].handler = h;
    ++app->route_count;
    return 0;
}

nnx_handler nnx_match_route(const nnx_app *app, nnx_method method, const char *path)
{
    size_t i;
    if (!app || !path) return NULL;
    for (i = 0; i < app->route_count; ++i)
        if (app->routes[i].method == method && strcmp(app->routes[i].path, path) == 0)
            return app->routes[i].handler;
    return NULL;
}

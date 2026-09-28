#include <stdlib.h>
#include "nnx_internal.h"

nnx_app *nnx_new(void) { return calloc(1, sizeof(nnx_app)); }

void nnx_free(nnx_app *app)
{
    size_t i;
    if (!app) return;
    for (i = 0; i < app->route_count; ++i) free(app->routes[i].path);
    free(app->routes);
    free(app);
}

int nnx_get(nnx_app *app, const char *path, nnx_handler h)
{ return nnx_add_route(app, NNX_GET, path, h); }

int nnx_post(nnx_app *app, const char *path, nnx_handler h)
{ return nnx_add_route(app, NNX_POST, path, h); }

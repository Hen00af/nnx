#include <stdlib.h>
#include "../internal/nnx_internal.h"

nnx_app *nnx_new(void) { return calloc(1, sizeof(nnx_app)); }

void nnx_free(nnx_app *app)
{
    size_t i;
    if (!app) return;
    for (i = 0; i < app->route_count; ++i) free(app->routes[i].path);
    free(app->routes); free(app);
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

#include <stdlib.h>
#include <string.h>
#include "../internal/nnx_internal.h"

static char *nnx_app_strdup(const char *s)
{
    size_t n; char *out;
    if (!s) return NULL;
    n = strlen(s) + 1;
    out = malloc(n);
    if (out) memcpy(out, s, n);
    return out;
}

static void nnx_free_middleware(nnx_middleware_entry *entries, size_t count)
{
    size_t i;
    for (i = 0; i < count; ++i)
        if (entries[i].destroy)
            entries[i].destroy(entries[i].data);
    free(entries);
}

nnx_app *nnx_new(void) { return calloc(1, sizeof(nnx_app)); }

void nnx_free(nnx_app *app)
{
    size_t i;
    nnx_group *group;
    nnx_group *next;
    if (!app) return;
    for (i = 0; i < app->route_count; ++i) free(app->routes[i].path);
    nnx_free_middleware(app->middleware, app->middleware_count);
    group = app->groups;
    while (group) {
        next = group->next;
        nnx_free_middleware(group->middleware, group->middleware_count);
        free(group->prefix);
        free(group);
        group = next;
    }
    free(app->routes);
    free(app);
}

#define NNX_ROUTE_FN(name, method) int name(nnx_app *app, const char *path, nnx_handler h) { return nnx_add_route(app, method, path, h); }

NNX_ROUTE_FN(nnx_get, NNX_GET)
NNX_ROUTE_FN(nnx_post, NNX_POST)
NNX_ROUTE_FN(nnx_put, NNX_PUT)
NNX_ROUTE_FN(nnx_patch, NNX_PATCH)
NNX_ROUTE_FN(nnx_delete, NNX_DELETE)
NNX_ROUTE_FN(nnx_head, NNX_HEAD)
NNX_ROUTE_FN(nnx_options, NNX_OPTIONS)
NNX_ROUTE_FN(nnx_any, NNX_METHOD_ANY)

static int nnx_add_middleware(nnx_middleware_entry **entries,
                              size_t *count, size_t *capacity,
                              nnx_middleware middleware)
{
    nnx_middleware_entry *next;
    size_t cap;
    if (!middleware.fn) return -1;
    if (*count == *capacity) {
        cap = *capacity ? *capacity * 2 : 4;
        next = realloc(*entries, cap * sizeof(*next));
        if (!next) {
            if (middleware.destroy) middleware.destroy(middleware.data);
            return -1;
        }
        *entries = next;
        *capacity = cap;
    }
    (*entries)[*count].fn = middleware.fn;
    (*entries)[*count].data = middleware.data;
    (*entries)[*count].destroy = middleware.destroy;
    ++*count;
    return 0;
}

int nnx_use(nnx_app *app, nnx_middleware middleware)
{
    if (!app) {
        if (middleware.destroy) middleware.destroy(middleware.data);
        return -1;
    }
    return nnx_add_middleware(&app->middleware, &app->middleware_count,
                              &app->middleware_capacity, middleware);
}

nnx_group *nnx_group_new(nnx_app *app, const char *prefix)
{
    nnx_group *group;
    if (!app || !prefix || prefix[0] != '/') return NULL;
    group = calloc(1, sizeof(*group));
    if (!group) return NULL;
    group->prefix = nnx_app_strdup(prefix);
    if (!group->prefix) {
        free(group);
        return NULL;
    }
    group->app = app;
    group->next = app->groups;
    app->groups = group;
    return group;
}

int nnx_group_use(nnx_group *group, nnx_middleware middleware)
{
    if (!group) {
        if (middleware.destroy) middleware.destroy(middleware.data);
        return -1;
    }
    return nnx_add_middleware(&group->middleware, &group->middleware_count,
                              &group->middleware_capacity, middleware);
}

#define NNX_GROUP_ROUTE_FN(name, method) int name(nnx_group *group, const char *path, nnx_handler h) { return nnx_add_group_route(group, method, path, h); }

NNX_GROUP_ROUTE_FN(nnx_group_get, NNX_GET)
NNX_GROUP_ROUTE_FN(nnx_group_post, NNX_POST)
NNX_GROUP_ROUTE_FN(nnx_group_put, NNX_PUT)
NNX_GROUP_ROUTE_FN(nnx_group_patch, NNX_PATCH)
NNX_GROUP_ROUTE_FN(nnx_group_delete, NNX_DELETE)
NNX_GROUP_ROUTE_FN(nnx_group_head, NNX_HEAD)
NNX_GROUP_ROUTE_FN(nnx_group_options, NNX_OPTIONS)
NNX_GROUP_ROUTE_FN(nnx_group_any, NNX_METHOD_ANY)

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

void nnx_set_error_handler(nnx_app *app, nnx_error_handler handler)
{
    if (app) app->error_handler = handler;
}

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
    if (!middleware.fn) {
        if (middleware.destroy) middleware.destroy(middleware.data);
        return -1;
    }
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

static char *nnx_join_prefix(const char *parent, const char *child)
{
    size_t pl, cl, skip = 0;
    char *out;

    if (!child || child[0] != '/') return NULL;
    if (!parent) return nnx_app_strdup(child);

    pl = strlen(parent);
    cl = strlen(child);
    if (pl == 1 && parent[0] == '/') pl = 0;
    else if (pl > 1 && parent[pl - 1] == '/') skip = 1;

    out = malloc(pl + cl + 1);
    if (!out) return NULL;
    if (pl) memcpy(out, parent, pl);
    memcpy(out + pl, child + skip, cl - skip + 1);
    return out;
}

static nnx_group *nnx_create_group(nnx_app *app, nnx_group *parent,
                                    const char *prefix)
{
    nnx_group *group;
    char *full;

    if (!app || !prefix || prefix[0] != '/') return NULL;
    full = nnx_join_prefix(parent ? parent->prefix : NULL, prefix);
    if (!full) return NULL;

    group = calloc(1, sizeof(*group));
    if (!group) {
        free(full);
        return NULL;
    }

    group->app = app;
    group->parent = parent;
    group->prefix = full;
    group->next = app->groups;
    app->groups = group;
    return group;
}

nnx_group *nnx_group_new(nnx_app *app, const char *prefix)
{
    return nnx_create_group(app, NULL, prefix);
}

nnx_group *nnx_group_group(nnx_group *parent, const char *prefix)
{
    if (!parent) return NULL;
    return nnx_create_group(parent->app, parent, prefix);
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

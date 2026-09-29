#include <stdlib.h>
#include <string.h>
#include "../internal/nnx_internal.h"

static char *nnx_strdup(const char *s)
{
    size_t n; char *p;
    if (!s) return NULL;
    n = strlen(s) + 1; p = malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

static int nnx_store_route(nnx_app *app, nnx_group *group, nnx_method method,
                           const char *path, nnx_handler h)
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
    app->routes[app->route_count].group = group;
    ++app->route_count; return 0;
}

int nnx_add_route(nnx_app *app, nnx_method method, const char *path, nnx_handler h)
{
    return nnx_store_route(app, NULL, method, path, h);
}

int nnx_add_group_route(nnx_group *group, nnx_method method,
                        const char *path, nnx_handler h)
{
    size_t prefix_len, path_len, skip = 0;
    char *full;
    int rc;

    if (!group || !group->app || !path || path[0] != '/') return -1;
    prefix_len = strlen(group->prefix);
    path_len = strlen(path);
    if (prefix_len > 1 && group->prefix[prefix_len - 1] == '/')
        skip = 1;
    if (prefix_len == 1 && group->prefix[0] == '/')
        prefix_len = 0;

    full = malloc(prefix_len + path_len + 1);
    if (!full) return -1;
    if (prefix_len) memcpy(full, group->prefix, prefix_len);
    memcpy(full + prefix_len, path + skip, path_len - skip + 1);

    rc = nnx_store_route(group->app, group, method, full, h);
    free(full);
    return rc;
}

static int method_matches(nnx_method route, nnx_method request)
{ return route == request || route == NNX_METHOD_ANY; }

static int param_match(const char *pattern, const char *path, nnx_ctx *ctx)
{
    const char *p = pattern, *s = path;
    ctx->param_count = 0;
    while (*p || *s) {
        if (*p == ':' && (p == pattern || p[-1] == '/')) {
            const char *name = ++p, *value = s; size_t nl, vl;
            while (*p && *p != '/') ++p;
            while (*s && *s != '/') ++s;
            nl = (size_t)(p - name); vl = (size_t)(s - value);
            if (!nl || !vl || ctx->param_count >= NNX_MAX_PARAMS ||
                nl >= NNX_PARAM_NAME_MAX || vl >= NNX_PARAM_VALUE_MAX) return 0;
            memcpy(ctx->params[ctx->param_count].name, name, nl);
            ctx->params[ctx->param_count].name[nl] = 0;
            memcpy(ctx->params[ctx->param_count].value, value, vl);
            ctx->params[ctx->param_count].value[vl] = 0;
            ++ctx->param_count;
        } else {
            if (*p != *s) return 0;
            ++p; ++s;
        }
    }
    return 1;
}

static int wildcard_match(const char *pattern, const char *path, nnx_ctx *ctx)
{
    const char *star = strchr(pattern, '*'); size_t prefix, rest;
    if (!star) return 0;
    prefix = (size_t)(star - pattern);
    if (strncmp(pattern, path, prefix) != 0) return 0;
    rest = strlen(path + prefix);
    if (rest >= sizeof(ctx->wildcard)) return 0;
    memcpy(ctx->wildcard, path + prefix, rest + 1);
    return 1;
}

nnx_handler nnx_match_route(const nnx_app *app, nnx_method method, const char *path, nnx_ctx *ctx)
{
    size_t i;
    if (!app || !path || !ctx) return NULL;
    ctx->param_count = 0; ctx->wildcard[0] = 0; ctx->group = NULL;

    /* Echo-like priority: static, then param, then wildcard. */
    for (i = 0; i < app->route_count; ++i)
        if (method_matches(app->routes[i].method, method) &&
            !strchr(app->routes[i].path, ':') && !strchr(app->routes[i].path, '*') &&
            strcmp(app->routes[i].path, path) == 0) {
            ctx->group = app->routes[i].group;
            return app->routes[i].handler;
        }

    for (i = 0; i < app->route_count; ++i) {
        if (!method_matches(app->routes[i].method, method) ||
            !strchr(app->routes[i].path, ':') || strchr(app->routes[i].path, '*')) continue;
        ctx->param_count = 0;
        if (param_match(app->routes[i].path, path, ctx)) {
            ctx->group = app->routes[i].group;
            return app->routes[i].handler;
        }
    }

    for (i = 0; i < app->route_count; ++i)
        if (method_matches(app->routes[i].method, method) &&
            wildcard_match(app->routes[i].path, path, ctx)) {
            ctx->group = app->routes[i].group;
            return app->routes[i].handler;
        }
    return NULL;
}

static int path_pattern_matches(const char *pattern, const char *path,
                                nnx_ctx *ctx)
{
    ctx->param_count = 0;
    ctx->wildcard[0] = 0;

    if (!strchr(pattern, ':') && !strchr(pattern, '*'))
        return strcmp(pattern, path) == 0;
    if (strchr(pattern, ':') && !strchr(pattern, '*'))
        return param_match(pattern, path, ctx);
    if (strchr(pattern, '*'))
        return wildcard_match(pattern, path, ctx);
    return 0;
}

static const nnx_route *find_path_route(const nnx_app *app,
                                        const char *path, nnx_ctx *ctx)
{
    size_t i;
    int pass;

    for (pass = 0; pass < 3; ++pass) {
        for (i = 0; i < app->route_count; ++i) {
            const char *pattern = app->routes[i].path;
            int has_param = strchr(pattern, ':') != NULL;
            int has_wildcard = strchr(pattern, '*') != NULL;

            if (pass == 0 && (has_param || has_wildcard)) continue;
            if (pass == 1 && (!has_param || has_wildcard)) continue;
            if (pass == 2 && !has_wildcard) continue;

            if (path_pattern_matches(pattern, path, ctx)) {
                ctx->group = app->routes[i].group;
                return &app->routes[i];
            }
        }
    }
    ctx->group = NULL;
    return NULL;
}

unsigned nnx_route_allowed_methods(const nnx_app *app, const char *path,
                                   nnx_ctx *ctx)
{
    size_t i;
    unsigned methods = 0;
    nnx_ctx scratch = {0};

    if (!app || !path || !ctx) return 0;
    if (!find_path_route(app, path, ctx)) return 0;

    for (i = 0; i < app->route_count; ++i) {
        if (!path_pattern_matches(app->routes[i].path, path, &scratch))
            continue;
        if (app->routes[i].method == NNX_METHOD_ANY)
            methods |= NNX_ALL_METHOD_BITS;
        else
            methods |= NNX_METHOD_BIT(app->routes[i].method);
    }

    if (methods & NNX_METHOD_BIT(NNX_GET))
        methods |= NNX_METHOD_BIT(NNX_HEAD);
    if (methods)
        methods |= NNX_METHOD_BIT(NNX_OPTIONS);
    return methods;
}

int nnx_route_path_exists(const nnx_app *app, const char *path)
{
    nnx_ctx scratch = {0};
    return nnx_route_allowed_methods(app, path, &scratch) != 0;
}

int nnx_set_allow_header(nnx_ctx *ctx, unsigned methods)
{
    static const struct {
        nnx_method method;
        const char *name;
    } names[] = {
        {NNX_GET, "GET"},
        {NNX_HEAD, "HEAD"},
        {NNX_POST, "POST"},
        {NNX_PUT, "PUT"},
        {NNX_PATCH, "PATCH"},
        {NNX_DELETE, "DELETE"},
        {NNX_OPTIONS, "OPTIONS"}
    };
    char allow[128];
    size_t used = 0;
    size_t i;

    allow[0] = 0;
    for (i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
        size_t len;
        if (!(methods & NNX_METHOD_BIT(names[i].method))) continue;
        len = strlen(names[i].name);
        if (used && used + 2 >= sizeof(allow)) return -1;
        if (used) {
            allow[used++] = ',';
            allow[used++] = ' ';
        }
        if (used + len >= sizeof(allow)) return -1;
        memcpy(allow + used, names[i].name, len);
        used += len;
        allow[used] = 0;
    }
    return nnx_set_header(ctx, "Allow", allow);
}

const char *nnx_param(nnx_ctx *ctx, const char *name)
{
    size_t i;
    if (!ctx || !name) return NULL;
    for (i = 0; i < ctx->param_count; ++i)
        if (strcmp(ctx->params[i].name, name) == 0) return ctx->params[i].value;
    return NULL;
}

const char *nnx_wildcard(nnx_ctx *ctx)
{ return ctx && ctx->wildcard[0] ? ctx->wildcard : NULL; }

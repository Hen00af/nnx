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
    ++app->route_count; return 0;
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
    ctx->param_count = 0; ctx->wildcard[0] = 0;

    /* Echo-like priority: static, then param, then wildcard. */
    for (i = 0; i < app->route_count; ++i)
        if (method_matches(app->routes[i].method, method) &&
            !strchr(app->routes[i].path, ':') && !strchr(app->routes[i].path, '*') &&
            strcmp(app->routes[i].path, path) == 0) return app->routes[i].handler;

    for (i = 0; i < app->route_count; ++i) {
        if (!method_matches(app->routes[i].method, method) ||
            !strchr(app->routes[i].path, ':') || strchr(app->routes[i].path, '*')) continue;
        ctx->param_count = 0;
        if (param_match(app->routes[i].path, path, ctx)) return app->routes[i].handler;
    }

    for (i = 0; i < app->route_count; ++i)
        if (method_matches(app->routes[i].method, method) &&
            wildcard_match(app->routes[i].path, path, ctx)) return app->routes[i].handler;
    return NULL;
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

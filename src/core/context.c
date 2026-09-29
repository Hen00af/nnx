#include <string.h>
#include "../internal/nnx_internal.h"

void nnx_ctx_init(nnx_ctx *ctx, void *request, const nnx_adapter *adapter,
                  const char *method_name, const char *path)
{
    ctx->adapter_request = request;
    ctx->adapter = adapter;
    ctx->response_sent = 0;
    ctx->response_status = 0;
    ctx->method_name = method_name;
    ctx->path = path;
    ctx->body = NULL;
    ctx->body_len = 0;
    ctx->param_count = 0;
    ctx->wildcard[0] = 0;
    ctx->app = NULL;
    ctx->endpoint = NULL;
    ctx->middleware_index = 0;
    ctx->group = NULL;
    ctx->group_depth = 0;
    ctx->group_index = 0;
    ctx->group_middleware_index = 0;
    ctx->request_id[0] = 0;
    ctx->pending_error_status = 0;
    ctx->value_count = 0;
}

void nnx_ctx_set_body(nnx_ctx *ctx, const void *body, size_t len)
{
    if (!ctx) return;
    ctx->body = body;
    ctx->body_len = len;
}

const char *nnx_method_name(nnx_ctx *ctx)
{ return ctx ? ctx->method_name : NULL; }

const char *nnx_path(nnx_ctx *ctx)
{ return ctx ? ctx->path : NULL; }

const char *nnx_query(nnx_ctx *ctx, const char *name)
{
    if (!ctx || !name || !ctx->adapter || !ctx->adapter->query) return NULL;
    return ctx->adapter->query(ctx->adapter_request, name);
}

const char *nnx_header(nnx_ctx *ctx, const char *name)
{
    if (!ctx || !name || !ctx->adapter || !ctx->adapter->header) return NULL;
    return ctx->adapter->header(ctx->adapter_request, name);
}

void *nnx_alloc(nnx_ctx *ctx, size_t size)
{
    if (!ctx || !size || !ctx->adapter || !ctx->adapter->alloc) return NULL;
    return ctx->adapter->alloc(ctx->adapter_request, size);
}

static int nnx_ascii_ieq_prefix(const char *s, const char *prefix)
{
    unsigned char a, b;
    if (!s || !prefix) return 0;
    while (*prefix) {
        if (!*s) return 0;
        a = (unsigned char)*s++;
        b = (unsigned char)*prefix++;
        if (a >= 'A' && a <= 'Z') a = (unsigned char)(a + ('a' - 'A'));
        if (b >= 'A' && b <= 'Z') b = (unsigned char)(b + ('a' - 'A'));
        if (a != b) return 0;
    }
    return 1;
}

static int nnx_hex_value(unsigned char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static char *nnx_url_decode(nnx_ctx *ctx, const char *src, size_t len)
{
    char *out;
    size_t i, n = 0;
    int hi, lo;

    out = nnx_alloc(ctx, len + 1);
    if (!out) return NULL;

    for (i = 0; i < len; ++i) {
        if (src[i] == '+') {
            out[n++] = ' ';
        } else if (src[i] == '%' && i + 2 < len &&
                   (hi = nnx_hex_value((unsigned char)src[i + 1])) >= 0 &&
                   (lo = nnx_hex_value((unsigned char)src[i + 2])) >= 0) {
            out[n++] = (char)((hi << 4) | lo);
            i += 2;
        } else {
            out[n++] = src[i];
        }
    }
    out[n] = 0;
    return out;
}

const char *nnx_cookie(nnx_ctx *ctx, const char *name)
{
    const char *cookie;
    const char *p;
    const char *key;
    const char *key_end;
    const char *value;
    const char *value_end;
    size_t name_len;
    char *out;

    if (!ctx || !name) return NULL;
    cookie = nnx_header(ctx, "Cookie");
    if (!cookie) return NULL;
    name_len = strlen(name);
    p = cookie;

    while (*p) {
        while (*p == ' ' || *p == '\t' || *p == ';') ++p;
        key = p;
        while (*p && *p != '=' && *p != ';') ++p;
        key_end = p;
        while (key_end > key && (key_end[-1] == ' ' || key_end[-1] == '\t'))
            --key_end;
        if (*p != '=') {
            while (*p && *p != ';') ++p;
            continue;
        }
        ++p;
        while (*p == ' ' || *p == '\t') ++p;
        value = p;
        while (*p && *p != ';') ++p;
        value_end = p;
        while (value_end > value &&
               (value_end[-1] == ' ' || value_end[-1] == '\t'))
            --value_end;

        if ((size_t)(key_end - key) == name_len &&
            memcmp(key, name, name_len) == 0) {
            size_t len = (size_t)(value_end - value);
            out = nnx_alloc(ctx, len + 1);
            if (!out) return NULL;
            if (len) memcpy(out, value, len);
            out[len] = 0;
            return out;
        }
    }
    return NULL;
}

const char *nnx_form(nnx_ctx *ctx, const char *name)
{
    const char *content_type;
    const char *body;
    const char *end;
    const char *pair_end;
    const char *eq;
    char *decoded_key;
    char *decoded_value;

    if (!ctx || !name || !ctx->body) return NULL;
    content_type = nnx_header(ctx, "Content-Type");
    if (!content_type ||
        !nnx_ascii_ieq_prefix(content_type, "application/x-www-form-urlencoded"))
        return NULL;

    body = (const char *)ctx->body;
    end = body + ctx->body_len;
    while (body < end) {
        pair_end = body;
        while (pair_end < end && *pair_end != '&') ++pair_end;
        eq = body;
        while (eq < pair_end && *eq != '=') ++eq;

        decoded_key = nnx_url_decode(ctx, body, (size_t)(eq - body));
        if (!decoded_key) return NULL;
        if (strcmp(decoded_key, name) == 0) {
            if (eq == pair_end)
                return nnx_url_decode(ctx, "", 0);
            decoded_value = nnx_url_decode(ctx, eq + 1,
                                           (size_t)(pair_end - (eq + 1)));
            return decoded_value;
        }
        body = pair_end < end ? pair_end + 1 : end;
    }
    return NULL;
}

const char *nnx_request_id(nnx_ctx *ctx)
{ return ctx && ctx->request_id[0] ? ctx->request_id : NULL; }

const char *nnx_host(nnx_ctx *ctx)
{ return nnx_header(ctx, "Host"); }

const char *nnx_scheme(nnx_ctx *ctx)
{
    if (!ctx || !ctx->adapter || !ctx->adapter->scheme) return NULL;
    return ctx->adapter->scheme(ctx->adapter_request);
}

const char *nnx_client_ip(nnx_ctx *ctx)
{
    if (!ctx || !ctx->adapter || !ctx->adapter->client_ip) return NULL;
    return ctx->adapter->client_ip(ctx->adapter_request);
}

int nnx_ctx_set(nnx_ctx *ctx, const char *key, void *value)
{
    size_t i;
    size_t n;

    if (!ctx || !key || !*key) return -1;
    n = strlen(key);
    if (n >= NNX_VALUE_KEY_MAX) return -1;

    for (i = 0; i < ctx->value_count; ++i) {
        if (strcmp(ctx->values[i].key, key) == 0) {
            ctx->values[i].value = value;
            return 0;
        }
    }

    if (ctx->value_count >= NNX_MAX_VALUES) return -1;
    memcpy(ctx->values[ctx->value_count].key, key, n + 1);
    ctx->values[ctx->value_count].value = value;
    ++ctx->value_count;
    return 0;
}

void *nnx_ctx_get(nnx_ctx *ctx, const char *key)
{
    size_t i;
    if (!ctx || !key) return NULL;
    for (i = 0; i < ctx->value_count; ++i)
        if (strcmp(ctx->values[i].key, key) == 0)
            return ctx->values[i].value;
    return NULL;
}

const void *nnx_body(nnx_ctx *ctx, size_t *len)
{
    if (len) *len = ctx ? ctx->body_len : 0;
    return ctx ? ctx->body : NULL;
}

const char *nnx_body_text(nnx_ctx *ctx)
{
    static const char empty[] = "";
    return ctx && ctx->body ? (const char *)ctx->body : empty;
}

int nnx_status(nnx_ctx *ctx)
{ return ctx ? ctx->response_status : 0; }

int nnx_log(nnx_ctx *ctx, const char *message)
{
    if (!ctx || !message || !ctx->adapter || !ctx->adapter->log) return -1;
    return ctx->adapter->log(ctx->adapter_request, message);
}

static void nnx_default_error_handler(nnx_ctx *ctx, int status)
{
    const char *message = "Internal Server Error\n";
    if (status == 404) message = "Not Found\n";
    else if (status == 405) message = "Method Not Allowed\n";
    else if (status == 413) message = "Payload Too Large\n";
    nnx_text(ctx, status, message);
}

static void nnx_invoke_error(nnx_ctx *ctx, int status)
{
    nnx_error_handler handler;
    if (!ctx || ctx->response_sent) return;
    handler = ctx->app ? ctx->app->error_handler : NULL;
    if (handler) handler(ctx, status);
    else nnx_default_error_handler(ctx, status);
}

static void nnx_error_endpoint(nnx_ctx *ctx)
{
    nnx_invoke_error(ctx, ctx->pending_error_status ?
                     ctx->pending_error_status : 500);
}

void nnx_next(nnx_ctx *ctx)
{
    nnx_middleware_entry *entry;
    if (!ctx || !ctx->app) return;
    if (ctx->middleware_index < ctx->app->middleware_count) {
        entry = &ctx->app->middleware[ctx->middleware_index++];
        entry->fn(ctx, entry->data);
        return;
    }
    while (ctx->group_index < ctx->group_depth) {
        const nnx_group *group = ctx->group_chain[ctx->group_index];
        if (ctx->group_middleware_index < group->middleware_count) {
            entry = &group->middleware[ctx->group_middleware_index++];
            entry->fn(ctx, entry->data);
            return;
        }
        ++ctx->group_index;
        ctx->group_middleware_index = 0;
    }
    if (ctx->endpoint) {
        nnx_handler endpoint = ctx->endpoint;
        ctx->endpoint = NULL;
        endpoint(ctx);
        if (!ctx->response_sent && ctx->pending_error_status == 0)
            nnx_invoke_error(ctx, 500);
    }
}

static int nnx_build_group_chain(nnx_ctx *ctx)
{
    const nnx_group *group;
    const nnx_group *reverse[NNX_MAX_GROUP_DEPTH];
    size_t depth = 0;
    size_t i;

    ctx->group_depth = 0;
    ctx->group_index = 0;
    ctx->group_middleware_index = 0;

    for (group = ctx->group; group; group = group->parent) {
        if (depth >= NNX_MAX_GROUP_DEPTH) return -1;
        reverse[depth++] = group;
    }

    for (i = 0; i < depth; ++i)
        ctx->group_chain[i] = reverse[depth - i - 1];
    ctx->group_depth = depth;
    return 0;
}

void nnx_dispatch(nnx_app *app, nnx_ctx *ctx, nnx_handler endpoint)
{
    if (!app || !ctx || !endpoint) return;
    ctx->app = app;
    ctx->endpoint = endpoint;
    ctx->middleware_index = 0;
    ctx->pending_error_status = 0;
    if (nnx_build_group_chain(ctx) != 0) {
        ctx->group = NULL;
        ctx->group_depth = 0;
        ctx->pending_error_status = 500;
        ctx->endpoint = nnx_error_endpoint;
    }
    nnx_next(ctx);
}

void nnx_dispatch_error(nnx_app *app, nnx_ctx *ctx, int status)
{
    if (!app || !ctx) return;
    ctx->group = NULL;
    ctx->group_depth = 0;
    ctx->group_index = 0;
    ctx->group_middleware_index = 0;
    ctx->pending_error_status = status;
    ctx->app = app;
    ctx->endpoint = nnx_error_endpoint;
    ctx->middleware_index = 0;
    ctx->group_middleware_index = 0;
    nnx_next(ctx);
}

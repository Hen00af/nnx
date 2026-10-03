#include <stdio.h>
#include <string.h>
#include "../internal/nnx_internal.h"

static int nnx_header_name_valid(const char *name)
{
    const unsigned char *p = (const unsigned char *)name;

    if (!p || !*p) return 0;
    for (; *p; ++p) {
        if ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') ||
            (*p >= '0' && *p <= '9') ||
            strchr("!#$%&'*+-.^_`|~", *p))
            continue;
        return 0;
    }
    return 1;
}

static int nnx_header_value_valid(const char *value)
{
    const unsigned char *p = (const unsigned char *)value;

    if (!p) return 0;
    for (; *p; ++p)
        if ((*p < 0x20 && *p != '\t') || *p == 0x7f)
            return 0;
    return 1;
}

int nnx_set_header(nnx_ctx *ctx, const char *name, const char *value)
{
    if (!ctx || !nnx_header_name_valid(name) ||
        !nnx_header_value_valid(value) || ctx->response_sent ||
        !ctx->adapter || !ctx->adapter->set_header)
        return -1;
    return ctx->adapter->set_header(ctx->adapter_request, name, value);
}

int nnx_blob(nnx_ctx *ctx, int status, const char *content_type,
             const void *data, size_t len)
{
    static const char empty = 0;
    if (!ctx || !nnx_header_value_valid(content_type) ||
        (!data && len != 0) || ctx->response_sent ||
        !ctx->adapter || !ctx->adapter->send)
        return -1;
    if (!data) data = &empty;
    if (ctx->adapter->send(ctx->adapter_request, status, content_type, data, len) != 0)
        return -1;
    ctx->response_sent = 1;
    ctx->response_status = status;
    return 0;
}

int nnx_text(nnx_ctx *ctx, int status, const char *body)
{
    if (!body) return -1;
    return nnx_blob(ctx, status, "text/plain; charset=utf-8", body, strlen(body));
}

int nnx_send(nnx_ctx *ctx, int status, const char *body)
{ return nnx_text(ctx, status, body); }

int nnx_json(nnx_ctx *ctx, int status, const char *json)
{
    if (!json) return -1;
    return nnx_blob(ctx, status, "application/json; charset=utf-8", json, strlen(json));
}

int nnx_html(nnx_ctx *ctx, int status, const char *html)
{
    if (!html) return -1;
    return nnx_blob(ctx, status, "text/html; charset=utf-8", html, strlen(html));
}

int nnx_redirect(nnx_ctx *ctx, int status, const char *location)
{
    if (!location || nnx_set_header(ctx, "Location", location) != 0) return -1;
    return nnx_text(ctx, status, "");
}

static int nnx_cookie_token_valid(const char *s)
{
    const unsigned char *p = (const unsigned char *)s;
    if (!s || !*s) return 0;
    while (*p) {
        if (*p <= 0x20 || *p >= 0x7f || *p == '=' || *p == ';' || *p == ',')
            return 0;
        ++p;
    }
    return 1;
}

static int nnx_cookie_value_valid(const char *s)
{
    const unsigned char *p = (const unsigned char *)s;
    if (!s) return 0;
    while (*p) {
        if (*p < 0x20 || *p >= 0x7f || *p == ';' || *p == '\r' || *p == '\n')
            return 0;
        ++p;
    }
    return 1;
}

int nnx_set_cookie(nnx_ctx *ctx, const nnx_cookie_config *cookie)
{
    char value[2048];
    size_t used;
    int n;

    if (!ctx || !cookie || !nnx_cookie_token_valid(cookie->name) ||
        !nnx_cookie_value_valid(cookie->value))
        return -1;
    if (cookie->same_site == NNX_SAME_SITE_NONE && !cookie->secure)
        return -1;

    n = snprintf(value, sizeof(value), "%s=%s", cookie->name, cookie->value);
    if (n < 0 || (size_t)n >= sizeof(value)) return -1;
    used = (size_t)n;

#define NNX_COOKIE_APPEND(...) do { \
    n = snprintf(value + used, sizeof(value) - used, __VA_ARGS__); \
    if (n < 0 || (size_t)n >= sizeof(value) - used) return -1; \
    used += (size_t)n; \
} while (0)

    if (cookie->path && *cookie->path)
        NNX_COOKIE_APPEND("; Path=%s", cookie->path);
    if (cookie->domain && *cookie->domain)
        NNX_COOKIE_APPEND("; Domain=%s", cookie->domain);
    if (cookie->max_age != 0)
        NNX_COOKIE_APPEND("; Max-Age=%d", cookie->max_age);
    if (cookie->secure)
        NNX_COOKIE_APPEND("; Secure");
    if (cookie->http_only)
        NNX_COOKIE_APPEND("; HttpOnly");

    switch (cookie->same_site) {
    case NNX_SAME_SITE_LAX:
        NNX_COOKIE_APPEND("; SameSite=Lax");
        break;
    case NNX_SAME_SITE_STRICT:
        NNX_COOKIE_APPEND("; SameSite=Strict");
        break;
    case NNX_SAME_SITE_NONE:
        NNX_COOKIE_APPEND("; SameSite=None");
        break;
    default:
        break;
    }

#undef NNX_COOKIE_APPEND
    return nnx_set_header(ctx, "Set-Cookie", value);
}

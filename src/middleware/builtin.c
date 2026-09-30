#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <nnx/middleware.h>
#include "../internal/nnx_internal.h"

typedef struct nnx_cors_state {
    char *origin;
    char *methods;
    char *headers;
} nnx_cors_state;

typedef struct nnx_basic_auth_state {
    char *username;
    char *password;
    char *realm;
} nnx_basic_auth_state;

static char *nnx_mw_strdup(const char *s)
{
    size_t n;
    char *out;
    if (!s) return NULL;
    n = strlen(s) + 1;
    out = malloc(n);
    if (out) memcpy(out, s, n);
    return out;
}

static long long nnx_now_ms(void)
{
    struct timespec ts;
    if (timespec_get(&ts, TIME_UTC) != TIME_UTC) return 0;
    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

static void logger_middleware(nnx_ctx *ctx, void *data)
{
    char line[512];
    long long started = nnx_now_ms();
    long long elapsed;
    int status;
    const char *method;
    const char *path;
    const char *request_id;
    (void)data;

    nnx_next(ctx);

    elapsed = nnx_now_ms() - started;
    status = nnx_status(ctx);
    if (!status) status = 500;
    method = nnx_method_name(ctx);
    path = nnx_path(ctx);
    request_id = nnx_request_id(ctx);

    if (request_id)
        snprintf(line, sizeof(line), "[NNX] %s %s %d %lldms request_id=%s",
                 method ? method : "-", path ? path : "-", status, elapsed, request_id);
    else
        snprintf(line, sizeof(line), "[NNX] %s %s %d %lldms",
                 method ? method : "-", path ? path : "-", status, elapsed);
    nnx_log(ctx, line);
}

nnx_middleware nnx_logger(void)
{
    nnx_middleware m = { logger_middleware, NULL, NULL };
    return m;
}

static atomic_ullong request_counter = 1;

static void request_id_middleware(nnx_ctx *ctx, void *data)
{
    const char *incoming;
    unsigned long long id;
    size_t n;
    (void)data;

    incoming = nnx_header(ctx, "X-Request-ID");
    if (incoming) {
        n = strlen(incoming);
        if (n < sizeof(ctx->request_id)) {
            memcpy(ctx->request_id, incoming, n + 1);
        }
    }
    if (!ctx->request_id[0]) {
        id = atomic_fetch_add(&request_counter, 1);
        snprintf(ctx->request_id, sizeof(ctx->request_id), "nnx-%016llx", id);
    }
    nnx_set_header(ctx, "X-Request-ID", ctx->request_id);
    nnx_next(ctx);
}

nnx_middleware nnx_request_id_middleware(void)
{
    nnx_middleware m = { request_id_middleware, NULL, NULL };
    return m;
}

static void cors_destroy(void *data)
{
    nnx_cors_state *state = data;
    if (!state) return;
    free(state->origin);
    free(state->methods);
    free(state->headers);
    free(state);
}

static void cors_middleware(nnx_ctx *ctx, void *data)
{
    nnx_cors_state *state = data;
    if (!state) {
        nnx_next(ctx);
        return;
    }
    nnx_set_header(ctx, "Access-Control-Allow-Origin", state->origin);
    nnx_set_header(ctx, "Access-Control-Allow-Methods", state->methods);
    nnx_set_header(ctx, "Access-Control-Allow-Headers", state->headers);
    if (nnx_method_name(ctx) && strcmp(nnx_method_name(ctx), "OPTIONS") == 0) {
        nnx_text(ctx, 204, "");
        return;
    }
    nnx_next(ctx);
}

nnx_middleware nnx_cors(nnx_cors_config config)
{
    nnx_middleware bad = { NULL, NULL, NULL };
    nnx_middleware out;
    nnx_cors_state *state = calloc(1, sizeof(*state));
    if (!state) return bad;

    state->origin = nnx_mw_strdup(config.allow_origin ? config.allow_origin : "*");
    state->methods = nnx_mw_strdup(config.allow_methods ? config.allow_methods :
        "GET,POST,PUT,PATCH,DELETE,HEAD,OPTIONS");
    state->headers = nnx_mw_strdup(config.allow_headers ? config.allow_headers :
        "Content-Type,Authorization");
    if (!state->origin || !state->methods || !state->headers) {
        cors_destroy(state);
        return bad;
    }
    out.fn = cors_middleware;
    out.data = state;
    out.destroy = cors_destroy;
    return out;
}

static int b64_value(unsigned char c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

static int decode_basic(const char *src, char *out, size_t cap)
{
    unsigned int acc = 0;
    int bits = 0;
    size_t n = 0;
    int v;

    while (*src && *src != '=') {
        v = b64_value((unsigned char)*src++);
        if (v < 0) return -1;
        acc = (acc << 6) | (unsigned int)v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            if (n + 1 >= cap) return -1;
            out[n++] = (char)((acc >> bits) & 0xff);
        }
    }
    out[n] = 0;
    return 0;
}

static int secure_equal(const char *a, const char *b)
{
    size_t al = strlen(a), bl = strlen(b), i, max = al > bl ? al : bl;
    unsigned int diff = (unsigned int)(al ^ bl);
    for (i = 0; i < max; ++i) {
        unsigned char ac = i < al ? (unsigned char)a[i] : 0;
        unsigned char bc = i < bl ? (unsigned char)b[i] : 0;
        diff |= ac ^ bc;
    }
    return diff == 0;
}

static void basic_auth_destroy(void *data)
{
    nnx_basic_auth_state *state = data;
    if (!state) return;
    free(state->username);
    free(state->password);
    free(state->realm);
    free(state);
}

static void basic_auth_middleware(nnx_ctx *ctx, void *data)
{
    nnx_basic_auth_state *state = data;
    const char *authorization;
    char decoded[512];
    char expected[512];
    char challenge[256];

    if (!state) {
        nnx_next(ctx);
        return;
    }

    authorization = nnx_header(ctx, "Authorization");
    if (authorization && strncmp(authorization, "Basic ", 6) == 0 &&
        decode_basic(authorization + 6, decoded, sizeof(decoded)) == 0) {
        snprintf(expected, sizeof(expected), "%s:%s", state->username, state->password);
        if (secure_equal(decoded, expected)) {
            nnx_next(ctx);
            return;
        }
    }

    snprintf(challenge, sizeof(challenge), "Basic realm=\"%s\"", state->realm);
    nnx_set_header(ctx, "WWW-Authenticate", challenge);
    nnx_text(ctx, 401, "Unauthorized");
}

nnx_middleware nnx_basic_auth(nnx_basic_auth_config config)
{
    nnx_middleware bad = { NULL, NULL, NULL };
    nnx_middleware out;
    nnx_basic_auth_state *state;

    if (!config.username || !config.password) return bad;
    state = calloc(1, sizeof(*state));
    if (!state) return bad;
    state->username = nnx_mw_strdup(config.username);
    state->password = nnx_mw_strdup(config.password);
    state->realm = nnx_mw_strdup(config.realm ? config.realm : "Restricted");
    if (!state->username || !state->password || !state->realm) {
        basic_auth_destroy(state);
        return bad;
    }

    out.fn = basic_auth_middleware;
    out.data = state;
    out.destroy = basic_auth_destroy;
    return out;
}

typedef struct nnx_secure_state {
    char *frame;
    char *referrer;
    char *csp;
    int nosniff;
    long hsts_max_age;
    int hsts_include_subdomains;
    int hsts_preload;
} nnx_secure_state;

static void secure_destroy(void *data)
{
    nnx_secure_state *state = data;
    if (!state) return;
    free(state->frame);
    free(state->referrer);
    free(state->csp);
    free(state);
}

static void secure_middleware(nnx_ctx *ctx, void *data)
{
    nnx_secure_state *state = data;
    char hsts[256];
    int n;

    if (!state) {
        nnx_next(ctx);
        return;
    }

    if (state->nosniff)
        nnx_set_header(ctx, "X-Content-Type-Options", "nosniff");
    if (state->frame)
        nnx_set_header(ctx, "X-Frame-Options", state->frame);
    if (state->referrer)
        nnx_set_header(ctx, "Referrer-Policy", state->referrer);
    if (state->csp)
        nnx_set_header(ctx, "Content-Security-Policy", state->csp);

    if (state->hsts_max_age > 0) {
        n = snprintf(hsts, sizeof(hsts), "max-age=%ld", state->hsts_max_age);
        if (n > 0 && (size_t)n < sizeof(hsts)) {
            if (state->hsts_include_subdomains &&
                (size_t)n + sizeof("; includeSubDomains") < sizeof(hsts)) {
                strcat(hsts, "; includeSubDomains");
            }
            if (state->hsts_preload &&
                strlen(hsts) + sizeof("; preload") < sizeof(hsts)) {
                strcat(hsts, "; preload");
            }
            nnx_set_header(ctx, "Strict-Transport-Security", hsts);
        }
    }

    nnx_next(ctx);
}

nnx_middleware nnx_secure(nnx_secure_config config)
{
    nnx_middleware bad = { NULL, NULL, NULL };
    nnx_middleware out;
    nnx_secure_state *state = calloc(1, sizeof(*state));
    if (!state) return bad;

    state->frame = nnx_mw_strdup(config.x_frame_options ?
        config.x_frame_options : "SAMEORIGIN");
    state->referrer = nnx_mw_strdup(config.referrer_policy ?
        config.referrer_policy : "no-referrer");
    if (config.content_security_policy)
        state->csp = nnx_mw_strdup(config.content_security_policy);
    state->nosniff = config.content_type_nosniff == 0 ? 1 :
                     config.content_type_nosniff > 0;
    state->hsts_max_age = config.hsts_max_age;
    state->hsts_include_subdomains = config.hsts_include_subdomains;
    state->hsts_preload = config.hsts_preload;

    if (!state->frame || !state->referrer ||
        (config.content_security_policy && !state->csp)) {
        secure_destroy(state);
        return bad;
    }

    out.fn = secure_middleware;
    out.data = state;
    out.destroy = secure_destroy;
    return out;
}

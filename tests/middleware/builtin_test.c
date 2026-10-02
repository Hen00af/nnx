#include <assert.h>
#include <string.h>
#include <nnx.h>
#include <nnx/middleware.h>
#include "../../src/internal/nnx_internal.h"

static char request_authorization[2048];
static char request_id_header[128];
static char last_log[512];
static char header_names[16][64];
static char header_values[16][256];
static int header_count;
static int endpoint_calls;
static int status_seen;

static int fake_send(void *request, int status, const char *type,
                     const void *body, size_t len)
{
    (void)request; (void)type; (void)body; (void)len;
    status_seen = status;
    return 0;
}

static const char *fake_query(void *request, const char *name)
{
    (void)request; (void)name;
    return NULL;
}

static const char *fake_header(void *request, const char *name)
{
    (void)request;
    if (strcmp(name, "Authorization") == 0 && request_authorization[0])
        return request_authorization;
    if (strcmp(name, "X-Request-ID") == 0 && request_id_header[0])
        return request_id_header;
    return NULL;
}

static int fake_set_header(void *request, const char *name, const char *value)
{
    (void)request;
    assert(header_count < 16);
    strcpy(header_names[header_count], name);
    strcpy(header_values[header_count], value);
    ++header_count;
    return 0;
}

static int fake_log(void *request, const char *message)
{
    (void)request;
    strcpy(last_log, message);
    return 0;
}

static const nnx_adapter fake = {
    fake_send, fake_query, fake_header, fake_set_header, fake_log,
    NULL, NULL, NULL
};

static void ok_endpoint(nnx_ctx *ctx)
{
    ++endpoint_calls;
    nnx_text(ctx, 200, "ok");
}

static void reset_state(void)
{
    request_authorization[0] = 0;
    request_id_header[0] = 0;
    last_log[0] = 0;
    header_count = 0;
    endpoint_calls = 0;
    status_seen = 0;
}

static int has_header(const char *name, const char *value)
{
    int i;
    for (i = 0; i < header_count; ++i)
        if (strcmp(header_names[i], name) == 0 &&
            strcmp(header_values[i], value) == 0)
            return 1;
    return 0;
}

static void test_request_id_and_logger(void)
{
    nnx_app *app = nnx_new();
    nnx_ctx ctx;
    reset_state();
    assert(app);
    assert(nnx_use(app, nnx_request_id_middleware()) == 0);
    assert(nnx_use(app, nnx_logger()) == 0);

    nnx_ctx_init(&ctx, NULL, &fake, "GET", "/hello");
    nnx_dispatch(app, &ctx, ok_endpoint);

    assert(endpoint_calls == 1);
    assert(status_seen == 200);
    assert(nnx_request_id(&ctx) != NULL);
    assert(strncmp(nnx_request_id(&ctx), "nnx-", 4) == 0);
    assert(has_header("X-Request-ID", nnx_request_id(&ctx)));
    assert(strstr(last_log, "[NNX] GET /hello 200") != NULL);
    nnx_free(app);
}

static void test_cors_preflight(void)
{
    nnx_app *app = nnx_new();
    nnx_ctx ctx;
    reset_state();
    assert(app);
    assert(nnx_use(app, nnx_cors((nnx_cors_config){0})) == 0);

    nnx_ctx_init(&ctx, NULL, &fake, "OPTIONS", "/api");
    nnx_dispatch(app, &ctx, ok_endpoint);

    assert(endpoint_calls == 0);
    assert(status_seen == 204);
    assert(has_header("Access-Control-Allow-Origin", "*"));
    nnx_free(app);
}

static void test_secure_headers(void)
{
    nnx_app *app = nnx_new();
    nnx_ctx ctx;
    nnx_secure_config config = {0};

    reset_state();
    assert(app);
    config.hsts_max_age = 31536000;
    config.hsts_include_subdomains = 1;
    config.hsts_preload = 1;
    config.content_security_policy = "default-src 'self'";

    assert(nnx_use(app, nnx_secure(config)) == 0);
    nnx_ctx_init(&ctx, NULL, &fake, "GET", "/secure");
    nnx_dispatch(app, &ctx, ok_endpoint);

    assert(endpoint_calls == 1);
    assert(status_seen == 200);
    assert(has_header("X-Content-Type-Options", "nosniff"));
    assert(has_header("X-Frame-Options", "SAMEORIGIN"));
    assert(has_header("Referrer-Policy", "no-referrer"));
    assert(has_header("Content-Security-Policy", "default-src 'self'"));
    assert(has_header("Strict-Transport-Security",
                      "max-age=31536000; includeSubDomains; preload"));
    nnx_free(app);
}


static void encode_basic_header(const char *plain, char *out, size_t cap)
{
    static const char table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t len = strlen(plain);
    size_t i = 0;
    size_t n = 0;

    assert(cap > 7);
    memcpy(out, "Basic ", 6);
    n = 6;

    while (i < len) {
        size_t remain = len - i;
        unsigned int a = (unsigned char)plain[i++];
        unsigned int b = remain > 1 ? (unsigned char)plain[i++] : 0;
        unsigned int c = remain > 2 ? (unsigned char)plain[i++] : 0;
        unsigned int triple = (a << 16) | (b << 8) | c;

        assert(n + 4 < cap);
        out[n++] = table[(triple >> 18) & 0x3f];
        out[n++] = table[(triple >> 12) & 0x3f];
        out[n++] = remain > 1 ? table[(triple >> 6) & 0x3f] : '=';
        out[n++] = remain > 2 ? table[triple & 0x3f] : '=';
    }
    out[n] = 0;
}

static void test_basic_auth(void)
{
    nnx_app *app;
    nnx_ctx ctx;
    nnx_basic_auth_config config = {"user", "pass", "nnx"};

    reset_state();
    app = nnx_new();
    assert(app);
    assert(nnx_use(app, nnx_basic_auth(config)) == 0);
    strcpy(request_authorization, "Basic dXNlcjpwYXNz");
    nnx_ctx_init(&ctx, NULL, &fake, "GET", "/private");
    nnx_dispatch(app, &ctx, ok_endpoint);
    assert(endpoint_calls == 1);
    assert(status_seen == 200);
    nnx_free(app);

    reset_state();
    app = nnx_new();
    assert(app);
    assert(nnx_use(app, nnx_basic_auth(config)) == 0);
    strcpy(request_authorization, "Basic dXNlcjpiYWQ=");
    nnx_ctx_init(&ctx, NULL, &fake, "GET", "/private");
    nnx_dispatch(app, &ctx, ok_endpoint);
    assert(endpoint_calls == 0);
    assert(status_seen == 401);
    assert(has_header("WWW-Authenticate", "Basic realm=\"nnx\""));
    nnx_free(app);
}



static void test_basic_auth_long_username_cannot_bypass(void)
{
    nnx_app *app;
    nnx_ctx ctx;
    char username[511];
    char bypass_plain[512];
    char valid_plain[520];
    nnx_basic_auth_config config;
    size_t i;

    for (i = 0; i < 510; ++i)
        username[i] = 'a';
    username[510] = 0;

    memcpy(bypass_plain, username, 510);
    bypass_plain[510] = ':';
    bypass_plain[511] = 0;

    memcpy(valid_plain, username, 510);
    memcpy(valid_plain + 510, ":secret", 8);

    config.username = username;
    config.password = "secret";
    config.realm = "nnx";

    reset_state();
    app = nnx_new();
    assert(app);
    assert(nnx_use(app, nnx_basic_auth(config)) == 0);
    encode_basic_header(bypass_plain, request_authorization,
                        sizeof(request_authorization));
    nnx_ctx_init(&ctx, NULL, &fake, "GET", "/private");
    nnx_dispatch(app, &ctx, ok_endpoint);
    assert(endpoint_calls == 0);
    assert(status_seen == 401);
    nnx_free(app);

    reset_state();
    app = nnx_new();
    assert(app);
    assert(nnx_use(app, nnx_basic_auth(config)) == 0);
    encode_basic_header(valid_plain, request_authorization,
                        sizeof(request_authorization));
    nnx_ctx_init(&ctx, NULL, &fake, "GET", "/private");
    nnx_dispatch(app, &ctx, ok_endpoint);
    assert(endpoint_calls == 1);
    assert(status_seen == 200);
    nnx_free(app);
}

static void test_body_limit(void)
{
    nnx_app *app = nnx_new();
    nnx_ctx ctx;
    nnx_middleware invalid;

    reset_state();
    assert(app);

    invalid = nnx_body_limit(0);
    assert(invalid.fn == NULL);
    assert(nnx_use(app, nnx_body_limit(16)) == 0);

    nnx_ctx_init(&ctx, NULL, &fake, "POST", "/upload");
    nnx_ctx_set_body(&ctx, "0123456789abcdef", 16);
    nnx_dispatch(app, &ctx, ok_endpoint);
    assert(endpoint_calls == 1);
    assert(status_seen == 200);

    reset_state();
    nnx_ctx_init(&ctx, NULL, &fake, "POST", "/upload");
    nnx_ctx_set_body(&ctx, "0123456789abcdefX", 17);
    nnx_dispatch(app, &ctx, ok_endpoint);
    assert(endpoint_calls == 0);
    assert(status_seen == 413);

    nnx_free(app);
}

int main(void)
{
    test_request_id_and_logger();
    test_cors_preflight();
    test_secure_headers();
    test_basic_auth();
    test_basic_auth_long_username_cannot_bypass();
    test_body_limit();
    return 0;
}

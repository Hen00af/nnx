#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "../../src/internal/nnx_internal.h"

static int status_seen;
static const char *type_seen;
static unsigned char body_seen[64];
static size_t len_seen;
static char header_name_seen[32];
static char header_value_seen[256];
static unsigned char request_alloc[4096];
static size_t request_alloc_used;

static int fake_send(void *request, int status, const char *type,
                     const void *body, size_t len)
{
    (void)request;
    status_seen = status;
    type_seen = type;
    len_seen = len;
    if (len) memcpy(body_seen, body, len);
    return 0;
}

static const char *fake_query(void *request, const char *name)
{
    (void)request;
    return strcmp(name, "q") == 0 ? "nnx" : NULL;
}

static const char *fake_header(void *request, const char *name)
{
    (void)request;
    if (strcmp(name, "X-Test") == 0) return "yes";
    if (strcmp(name, "Cookie") == 0) return "session=abc123; theme=dark";
    if (strcmp(name, "Content-Type") == 0)
        return "application/x-www-form-urlencoded; charset=utf-8";
    if (strcmp(name, "Host") == 0) return "example.test";
    return NULL;
}

static int fake_set_header(void *request, const char *name, const char *value)
{
    (void)request;
    if (strlen(name) >= sizeof(header_name_seen) ||
        strlen(value) >= sizeof(header_value_seen))
        return -1;
    memcpy(header_name_seen, name, strlen(name) + 1);
    memcpy(header_value_seen, value, strlen(value) + 1);
    return 0;
}

static int fake_log(void *request, const char *message)
{
    (void)request; (void)message;
    return 0;
}

static void *fake_alloc(void *request, size_t size)
{
    void *out;
    (void)request;
    if (size > sizeof(request_alloc) - request_alloc_used)
        return NULL;
    out = request_alloc + request_alloc_used;
    request_alloc_used += size;
    return out;
}

static const char *fake_client_ip(void *request)
{
    (void)request;
    return "127.0.0.1";
}

static const char *fake_scheme(void *request)
{
    (void)request;
    return "https";
}

int main(void)
{
    nnx_ctx ctx;
    size_t body_len;
    request_alloc_used = 0;
    const nnx_adapter fake = {
        fake_send, fake_query, fake_header, fake_set_header, fake_log,
        fake_alloc, fake_client_ip, fake_scheme
    };

    nnx_ctx_init(&ctx, NULL, &fake, "POST", "/hello");
    assert(strcmp(nnx_method_name(&ctx), "POST") == 0);
    assert(strcmp(nnx_path(&ctx), "/hello") == 0);
    assert(strcmp(nnx_query(&ctx, "q"), "nnx") == 0);
    assert(strcmp(nnx_header(&ctx, "X-Test"), "yes") == 0);
    assert(strcmp(nnx_host(&ctx), "example.test") == 0);
    assert(strcmp(nnx_client_ip(&ctx), "127.0.0.1") == 0);
    assert(strcmp(nnx_scheme(&ctx), "https") == 0);

    {
        int user_id = 42;
        assert(nnx_ctx_set(&ctx, "user_id", &user_id) == 0);
        assert(nnx_ctx_get(&ctx, "user_id") == &user_id);
        assert(nnx_ctx_set(&ctx, "user_id", NULL) == 0);
        assert(nnx_ctx_get(&ctx, "user_id") == NULL);
    }

    nnx_ctx_set_body(&ctx, "name=Seiya+Hattori&lang=C%2B%2B", 31);
    assert(nnx_body(&ctx, &body_len) != NULL);
    assert(body_len == 31);
    assert(strcmp(nnx_form(&ctx, "name"), "Seiya Hattori") == 0);
    assert(strcmp(nnx_form(&ctx, "lang"), "C++") == 0);
    assert(strcmp(nnx_cookie(&ctx, "session"), "abc123") == 0);
    assert(strcmp(nnx_cookie(&ctx, "theme"), "dark") == 0);

    assert(nnx_json(&ctx, 201, "{\"ok\":true}") == 0);
    assert(status_seen == 201);
    assert(strcmp(type_seen, "application/json; charset=utf-8") == 0);
    assert(len_seen == strlen("{\"ok\":true}"));
    assert(memcmp(body_seen, "{\"ok\":true}", len_seen) == 0);
    assert(nnx_text(&ctx, 200, "twice") == -1);

    nnx_ctx_init(&ctx, NULL, &fake, "GET", "/cookie");
    assert(nnx_set_cookie(&ctx, &(nnx_cookie_config){
        "session", "abc123", "/", NULL, 3600, 1, 1, NNX_SAME_SITE_LAX
    }) == 0);
    assert(strcmp(header_name_seen, "Set-Cookie") == 0);
    assert(strcmp(header_value_seen,
        "session=abc123; Path=/; Max-Age=3600; Secure; HttpOnly; SameSite=Lax") == 0);

    assert(nnx_set_cookie(&ctx, &(nnx_cookie_config){
        "bad", "value", "/", NULL, 0, 0, 0, NNX_SAME_SITE_NONE
    }) == -1);

    nnx_ctx_init(&ctx, NULL, &fake, "GET", "/old");
    assert(nnx_redirect(&ctx, 302, "/new") == 0);
    assert(strcmp(header_name_seen, "Location") == 0);
    assert(strcmp(header_value_seen, "/new") == 0);
    assert(status_seen == 302);

    nnx_ctx_init(&ctx, NULL, &fake, "GET", "/headers");
    assert(nnx_set_header(&ctx, "X-Test", "safe") == 0);
    assert(nnx_set_header(&ctx, "X-Test", "safe\r\nX-Injected: yes") == -1);
    assert(nnx_set_header(&ctx, "X-Bad\nName", "value") == -1);
    assert(nnx_set_header(&ctx, "Bad Name", "value") == -1);
    assert(strcmp(header_name_seen, "X-Test") == 0);
    assert(strcmp(header_value_seen, "safe") == 0);
    assert(nnx_blob(&ctx, 200, "text/plain\r\nX-Injected: yes", "ok", 2) == -1);
    assert(nnx_text(&ctx, 200, "ok") == 0);
    return 0;
}

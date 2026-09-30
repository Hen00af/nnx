#include <assert.h>
#include <string.h>
#include "../../src/internal/nnx_internal.h"

static int status_seen;
static const char *type_seen;
static unsigned char body_seen[64];
static size_t len_seen;
static char header_name_seen[32];
static char header_value_seen[64];

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
    return strcmp(name, "X-Test") == 0 ? "yes" : NULL;
}

static int fake_set_header(void *request, const char *name, const char *value)
{
    (void)request;
    strcpy(header_name_seen, name);
    strcpy(header_value_seen, value);
    return 0;
}

static int fake_log(void *request, const char *message)
{
    (void)request; (void)message;
    return 0;
}

int main(void)
{
    nnx_ctx ctx;
    size_t body_len;
    const nnx_adapter fake = {
        fake_send, fake_query, fake_header, fake_set_header, fake_log
    };

    nnx_ctx_init(&ctx, NULL, &fake, "POST", "/hello");
    assert(strcmp(nnx_method_name(&ctx), "POST") == 0);
    assert(strcmp(nnx_path(&ctx), "/hello") == 0);
    assert(strcmp(nnx_query(&ctx, "q"), "nnx") == 0);
    assert(strcmp(nnx_header(&ctx, "X-Test"), "yes") == 0);

    nnx_ctx_set_body(&ctx, "payload", 7);
    assert(nnx_body(&ctx, &body_len) != NULL);
    assert(body_len == 7);
    assert(strcmp(nnx_body_text(&ctx), "payload") == 0);

    assert(nnx_json(&ctx, 201, "{\"ok\":true}") == 0);
    assert(status_seen == 201);
    assert(strcmp(type_seen, "application/json; charset=utf-8") == 0);
    assert(len_seen == strlen("{\"ok\":true}"));
    assert(memcmp(body_seen, "{\"ok\":true}", len_seen) == 0);
    assert(nnx_text(&ctx, 200, "twice") == -1);

    nnx_ctx_init(&ctx, NULL, &fake, "GET", "/old");
    assert(nnx_redirect(&ctx, 302, "/new") == 0);
    assert(strcmp(header_name_seen, "Location") == 0);
    assert(strcmp(header_value_seen, "/new") == 0);
    assert(status_seen == 302);
    return 0;
}

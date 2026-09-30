#include <assert.h>
#include <string.h>
#include "../../src/internal/nnx_internal.h"

static int status_seen;
static char body_seen[128];

static int fake_send(void *request, int status, const char *type,
                     const void *body, size_t len)
{
    (void)request;
    (void)type;
    status_seen = status;
    assert(len < sizeof(body_seen));
    memcpy(body_seen, body, len);
    body_seen[len] = 0;
    return 0;
}

static const nnx_adapter fake = {
    fake_send, NULL, NULL, NULL, NULL, NULL
};

static void empty_handler(nnx_ctx *ctx)
{
    (void)ctx;
}

static void custom_error(nnx_ctx *ctx, int status)
{
    if (status == 404)
        nnx_json(ctx, 404, "{\"error\":\"missing\"}");
    else
        nnx_text(ctx, status, "custom");
}

int main(void)
{
    nnx_app *app = nnx_new();
    nnx_ctx ctx;

    assert(app);

    nnx_ctx_init(&ctx, NULL, &fake, "GET", "/missing");
    nnx_dispatch_error(app, &ctx, 404);
    assert(status_seen == 404);
    assert(strcmp(body_seen, "Not Found\n") == 0);

    status_seen = 0;
    body_seen[0] = 0;
    nnx_ctx_init(&ctx, NULL, &fake, "GET", "/empty");
    nnx_dispatch(app, &ctx, empty_handler);
    assert(status_seen == 500);
    assert(strcmp(body_seen, "Internal Server Error\n") == 0);

    nnx_set_error_handler(app, custom_error);
    status_seen = 0;
    body_seen[0] = 0;
    nnx_ctx_init(&ctx, NULL, &fake, "GET", "/missing");
    nnx_dispatch_error(app, &ctx, 404);
    assert(status_seen == 404);
    assert(strcmp(body_seen, "{\"error\":\"missing\"}") == 0);

    nnx_free(app);
    return 0;
}

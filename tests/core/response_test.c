#include <assert.h>
#include <string.h>
#include "../../src/internal/nnx_internal.h"

static int status_seen;
static char body_seen[64];

static int fake_send(void *request, int status, const char *body)
{
    (void)request;
    status_seen = status;
    strcpy(body_seen, body);
    return 0;
}

int main(void)
{
    nnx_ctx ctx;
    const nnx_adapter fake = { fake_send };
    nnx_ctx_init(&ctx, NULL, &fake);
    assert(nnx_send(&ctx, 201, "created") == 0);
    assert(status_seen == 201);
    assert(strcmp(body_seen, "created") == 0);
    assert(ctx.response_sent == 1);
    assert(ctx.response_status == 201);
    assert(nnx_send(&ctx, 200, "twice") == -1);
    return 0;
}

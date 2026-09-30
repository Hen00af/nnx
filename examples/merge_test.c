#include <stdio.h>
#include <string.h>
#include <nnx.h>
#include <nnx/middleware.h>

static void custom_error(nnx_ctx *ctx, int status)
{
    char json[160];
    snprintf(json, sizeof(json),
             "{\"error\":%d,\"request_id\":\"%s\"}\n",
             status, nnx_request_id(ctx) ? nnx_request_id(ctx) : "");
    nnx_json(ctx, status, json);
}

static void root(nnx_ctx *ctx)
{
    nnx_html(ctx, 200, "<h1>nnx works</h1><p>Powered by Nginx + nnx</p>");
}

static void user(nnx_ctx *ctx)
{
    char json[256];
    const char *id = nnx_param(ctx, "id");
    snprintf(json, sizeof(json),
             "{\"id\":\"%s\",\"method\":\"%s\",\"path\":\"%s\"}\n",
             id ? id : "",
             nnx_method_name(ctx) ? nnx_method_name(ctx) : "",
             nnx_path(ctx) ? nnx_path(ctx) : "");
    nnx_json(ctx, 200, json);
}

static void search(nnx_ctx *ctx)
{
    const char *q = nnx_query(ctx, "q");
    nnx_text(ctx, 200, q ? q : "(empty)");
}

static void echo_body(nnx_ctx *ctx)
{
    size_t len = 0;
    const void *body = nnx_body(ctx, &len);
    nnx_blob(ctx, 200, "application/octet-stream", body, len);
}

static void form(nnx_ctx *ctx)
{
    const char *name = nnx_form(ctx, "name");
    nnx_text(ctx, 200, name ? name : "(missing)");
}

static void set_cookie(nnx_ctx *ctx)
{
    nnx_cookie_config cookie = {
        .name = "nnx_session",
        .value = "hello",
        .path = "/",
        .domain = NULL,
        .max_age = 3600,
        .secure = 0,
        .http_only = 1,
        .same_site = NNX_SAME_SITE_LAX
    };
    if (nnx_set_cookie(ctx, &cookie) != 0) {
        nnx_text(ctx, 500, "cookie failed");
        return;
    }
    nnx_text(ctx, 200, "cookie set");
}

static void read_cookie(nnx_ctx *ctx)
{
    const char *value = nnx_cookie(ctx, "nnx_session");
    nnx_text(ctx, 200, value ? value : "(no cookie)");
}

static void metadata(nnx_ctx *ctx)
{
    char json[512];
    snprintf(json, sizeof(json),
             "{\"host\":\"%s\",\"scheme\":\"%s\",\"ip\":\"%s\",\"request_id\":\"%s\"}\n",
             nnx_host(ctx) ? nnx_host(ctx) : "",
             nnx_scheme(ctx) ? nnx_scheme(ctx) : "",
             nnx_client_ip(ctx) ? nnx_client_ip(ctx) : "",
             nnx_request_id(ctx) ? nnx_request_id(ctx) : "");
    nnx_json(ctx, 200, json);
}

static void ctx_store(nnx_ctx *ctx)
{
    char *message = nnx_alloc(ctx, 32);
    if (!message) {
        nnx_text(ctx, 500, "alloc failed");
        return;
    }
    strcpy(message, "request scoped memory");
    if (nnx_ctx_set(ctx, "message", message) != 0) {
        nnx_text(ctx, 500, "ctx set failed");
        return;
    }
    nnx_text(ctx, 200, (const char *)nnx_ctx_get(ctx, "message"));
}

static void grouped(nnx_ctx *ctx)
{
    const char *id = nnx_param(ctx, "id");
    nnx_text(ctx, 200, id ? id : "");
}

static void group_marker(nnx_ctx *ctx, void *data)
{
    nnx_set_header(ctx, "X-NNX-Group", (const char *)data);
    nnx_next(ctx);
}

static nnx_middleware marker(const char *value)
{
    nnx_middleware m = { group_marker, (void *)value, NULL };
    return m;
}

int nnx_register(nnx_app *app)
{
    nnx_group *api;
    nnx_group *v1;

    nnx_set_error_handler(app, custom_error);

    if (nnx_use(app, nnx_logger()) != 0) return -1;
    if (nnx_use(app, nnx_request_id_middleware()) != 0) return -1;
    if (nnx_use(app, nnx_secure((nnx_secure_config){
        .x_frame_options = "SAMEORIGIN",
        .referrer_policy = "no-referrer",
        .content_security_policy = NULL,
        .content_type_nosniff = 1,
        .hsts_max_age = 0,
        .hsts_include_subdomains = 0,
        .hsts_preload = 0
    })) != 0) return -1;

    if (nnx_get(app, "/", root) != 0) return -1;
    if (nnx_get(app, "/users/:id", user) != 0) return -1;
    if (nnx_get(app, "/search", search) != 0) return -1;
    if (nnx_post(app, "/echo", echo_body) != 0) return -1;
    if (nnx_post(app, "/form", form) != 0) return -1;
    if (nnx_get(app, "/cookie", set_cookie) != 0) return -1;
    if (nnx_get(app, "/cookie/read", read_cookie) != 0) return -1;
    if (nnx_get(app, "/metadata", metadata) != 0) return -1;
    if (nnx_get(app, "/ctx", ctx_store) != 0) return -1;

    api = nnx_group_new(app, "/api");
    if (!api) return -1;
    if (nnx_group_use(api, marker("api")) != 0) return -1;

    v1 = nnx_group_group(api, "/v1");
    if (!v1) return -1;
    if (nnx_group_get(v1, "/users/:id", grouped) != 0) return -1;

    return 0;
}

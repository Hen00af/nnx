#ifndef NNX_H
#define NNX_H

#include <stddef.h>

typedef struct nnx_app nnx_app;
typedef struct nnx_ctx nnx_ctx;
typedef struct nnx_group nnx_group;
typedef void (*nnx_handler)(nnx_ctx *ctx);
typedef void (*nnx_error_handler)(nnx_ctx *ctx, int status);
typedef void (*nnx_middleware_fn)(nnx_ctx *ctx, void *data);

typedef struct nnx_middleware {
    nnx_middleware_fn fn;
    void *data;
    void (*destroy)(void *data);
} nnx_middleware;

typedef enum nnx_same_site {
    NNX_SAME_SITE_DEFAULT = 0,
    NNX_SAME_SITE_LAX,
    NNX_SAME_SITE_STRICT,
    NNX_SAME_SITE_NONE
} nnx_same_site;

typedef struct nnx_cookie_config {
    const char *name;
    const char *value;
    const char *path;
    const char *domain;
    int max_age;
    int secure;
    int http_only;
    nnx_same_site same_site;
} nnx_cookie_config;

nnx_app *nnx_new(void);
void nnx_free(nnx_app *app);
void nnx_set_error_handler(nnx_app *app, nnx_error_handler handler);

int nnx_get(nnx_app *, const char *, nnx_handler);
int nnx_post(nnx_app *, const char *, nnx_handler);
int nnx_put(nnx_app *, const char *, nnx_handler);
int nnx_patch(nnx_app *, const char *, nnx_handler);
int nnx_delete(nnx_app *, const char *, nnx_handler);
int nnx_head(nnx_app *, const char *, nnx_handler);
int nnx_options(nnx_app *, const char *, nnx_handler);
int nnx_any(nnx_app *, const char *, nnx_handler);

nnx_group *nnx_group_new(nnx_app *, const char *prefix);
int nnx_group_use(nnx_group *, nnx_middleware);
int nnx_group_get(nnx_group *, const char *, nnx_handler);
int nnx_group_post(nnx_group *, const char *, nnx_handler);
int nnx_group_put(nnx_group *, const char *, nnx_handler);
int nnx_group_patch(nnx_group *, const char *, nnx_handler);
int nnx_group_delete(nnx_group *, const char *, nnx_handler);
int nnx_group_head(nnx_group *, const char *, nnx_handler);
int nnx_group_options(nnx_group *, const char *, nnx_handler);
int nnx_group_any(nnx_group *, const char *, nnx_handler);

int nnx_use(nnx_app *, nnx_middleware);
void nnx_next(nnx_ctx *);

const char *nnx_method_name(nnx_ctx *);
const char *nnx_path(nnx_ctx *);
const char *nnx_param(nnx_ctx *, const char *name);
const char *nnx_wildcard(nnx_ctx *);
const char *nnx_query(nnx_ctx *, const char *name);
const char *nnx_header(nnx_ctx *, const char *name);
const char *nnx_cookie(nnx_ctx *, const char *name);
const char *nnx_form(nnx_ctx *, const char *name);
const char *nnx_request_id(nnx_ctx *);
const char *nnx_host(nnx_ctx *);
const char *nnx_scheme(nnx_ctx *);
const char *nnx_client_ip(nnx_ctx *);
int nnx_ctx_set(nnx_ctx *, const char *key, void *value);
void *nnx_ctx_get(nnx_ctx *, const char *key);
void *nnx_alloc(nnx_ctx *, size_t size);
const void *nnx_body(nnx_ctx *, size_t *len);
const char *nnx_body_text(nnx_ctx *);
int nnx_status(nnx_ctx *);
int nnx_log(nnx_ctx *, const char *message);

int nnx_set_header(nnx_ctx *, const char *name, const char *value);
int nnx_set_cookie(nnx_ctx *, const nnx_cookie_config *cookie);
int nnx_send(nnx_ctx *, int status, const char *body);
int nnx_text(nnx_ctx *, int status, const char *body);
int nnx_json(nnx_ctx *, int status, const char *json);
int nnx_html(nnx_ctx *, int status, const char *html);
int nnx_blob(nnx_ctx *, int status, const char *content_type,
             const void *data, size_t len);
int nnx_redirect(nnx_ctx *, int status, const char *location);

int nnx_register(nnx_app *app);

#endif

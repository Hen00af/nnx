#ifndef NNX_H
#define NNX_H

#include <stddef.h>

typedef struct nnx_app nnx_app;
typedef struct nnx_ctx nnx_ctx;
typedef void (*nnx_handler)(nnx_ctx *ctx);
typedef void (*nnx_middleware_fn)(nnx_ctx *ctx, void *data);

nnx_app *nnx_new(void);
void nnx_free(nnx_app *app);

int nnx_get(nnx_app *, const char *, nnx_handler);
int nnx_post(nnx_app *, const char *, nnx_handler);
int nnx_put(nnx_app *, const char *, nnx_handler);
int nnx_patch(nnx_app *, const char *, nnx_handler);
int nnx_delete(nnx_app *, const char *, nnx_handler);
int nnx_head(nnx_app *, const char *, nnx_handler);
int nnx_options(nnx_app *, const char *, nnx_handler);
int nnx_any(nnx_app *, const char *, nnx_handler);

int nnx_use(nnx_app *, nnx_middleware_fn, void *data);
void nnx_next(nnx_ctx *);

const char *nnx_param(nnx_ctx *, const char *name);
const char *nnx_wildcard(nnx_ctx *);
int nnx_send(nnx_ctx *, int status, const char *body);

int nnx_register(nnx_app *app);

#endif

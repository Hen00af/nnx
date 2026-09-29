#ifndef NNX_H
#define NNX_H

#include <stddef.h>

typedef struct nnx_app nnx_app;
typedef struct nnx_ctx nnx_ctx;
typedef void (*nnx_handler)(nnx_ctx *ctx);
typedef void (*nnx_middleware)(nnx_ctx *ctx);

nnx_app *nnx_new(void);
void nnx_free(nnx_app *app);

int nnx_get(nnx_app *app, const char *path, nnx_handler handler);
int nnx_post(nnx_app *app, const char *path, nnx_handler handler);
int nnx_use(nnx_app *app, nnx_middleware before, nnx_middleware after);

const char *nnx_param(nnx_ctx *ctx, const char *name);
const char *nnx_query(nnx_ctx *ctx, const char *name);
const char *nnx_header(nnx_ctx *ctx, const char *name);
const char *nnx_method_name(nnx_ctx *ctx);
const char *nnx_path(nnx_ctx *ctx);

int nnx_send(nnx_ctx *ctx, int status, const char *body);
int nnx_text(nnx_ctx *ctx, int status, const char *body);
int nnx_json(nnx_ctx *ctx, int status, const char *json);

int nnx_register(nnx_app *app);

#endif

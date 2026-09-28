#ifndef NNX_H
#define NNX_H

typedef struct nnx_app nnx_app;
typedef struct nnx_ctx nnx_ctx;
typedef void (*nnx_handler)(nnx_ctx *ctx);

nnx_app *nnx_new(void);
void nnx_free(nnx_app *app);
int nnx_get(nnx_app *app, const char *path, nnx_handler handler);
int nnx_post(nnx_app *app, const char *path, nnx_handler handler);
int nnx_send(nnx_ctx *ctx, int status, const char *body);
int nnx_run(nnx_app *app, int port);

#endif

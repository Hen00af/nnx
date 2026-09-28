#ifndef NNX_INTERNAL_H
#define NNX_INTERNAL_H
#include <stddef.h>
#include <nnx.h>

typedef enum nnx_method { NNX_GET, NNX_POST } nnx_method;
typedef struct nnx_route {
    nnx_method method;
    char *path;
    nnx_handler handler;
} nnx_route;

struct nnx_app {
    nnx_route *routes;
    size_t route_count;
    size_t route_capacity;
};

struct nnx_ctx { void *native_request; };

int nnx_add_route(nnx_app *app, nnx_method method, const char *path, nnx_handler handler);
nnx_handler nnx_match_route(const nnx_app *app, nnx_method method, const char *path);
#endif

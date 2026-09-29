#ifndef NNX_INTERNAL_H
#define NNX_INTERNAL_H
#include <stddef.h>
#include <nnx.h>

#define NNX_MAX_PARAMS 16

typedef enum nnx_method { NNX_GET, NNX_POST } nnx_method;
typedef struct nnx_route { nnx_method method; char *path; nnx_handler handler; } nnx_route;
typedef struct nnx_middleware_pair { nnx_middleware before; nnx_middleware after; } nnx_middleware_pair;
typedef struct nnx_param_pair { char *name; char *value; } nnx_param_pair;

struct nnx_app {
    nnx_route *routes;
    size_t route_count;
    size_t route_capacity;
    nnx_middleware_pair *middleware;
    size_t middleware_count;
    size_t middleware_capacity;
};

struct nnx_ctx {
    void *native_request;
    int response_sent;
    int response_status;
    const char *method_name;
    const char *path;
    nnx_param_pair params[NNX_MAX_PARAMS];
    size_t param_count;
};

int nnx_add_route(nnx_app *, nnx_method, const char *, nnx_handler);
nnx_handler nnx_match_route(const nnx_app *, nnx_method, const char *, nnx_ctx *);
int nnx_adapter_send(nnx_ctx *, int, const char *, const char *);
const char *nnx_adapter_query(nnx_ctx *, const char *);
const char *nnx_adapter_header(nnx_ctx *, const char *);

#endif

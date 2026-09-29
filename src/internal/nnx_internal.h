#ifndef NNX_INTERNAL_H
#define NNX_INTERNAL_H

#include <stddef.h>
#include <nnx.h>

#define NNX_MAX_PARAMS 16
#define NNX_PARAM_NAME_MAX 64
#define NNX_PARAM_VALUE_MAX 256

typedef enum nnx_method {
    NNX_GET, NNX_POST, NNX_PUT, NNX_PATCH,
    NNX_DELETE, NNX_HEAD, NNX_OPTIONS, NNX_METHOD_ANY
} nnx_method;

typedef struct nnx_route { nnx_method method; char *path; nnx_handler handler; } nnx_route;
typedef struct nnx_param_pair {
    char name[NNX_PARAM_NAME_MAX];
    char value[NNX_PARAM_VALUE_MAX];
} nnx_param_pair;

typedef struct nnx_adapter {
    int (*send)(void *request, int status, const char *body);
} nnx_adapter;

struct nnx_app {
    nnx_route *routes;
    size_t route_count;
    size_t route_capacity;
};

struct nnx_ctx {
    void *adapter_request;
    const nnx_adapter *adapter;
    int response_sent;
    int response_status;
    nnx_param_pair params[NNX_MAX_PARAMS];
    size_t param_count;
    char wildcard[NNX_PARAM_VALUE_MAX];
};

int nnx_add_route(nnx_app *, nnx_method, const char *, nnx_handler);
nnx_handler nnx_match_route(const nnx_app *, nnx_method, const char *, nnx_ctx *);
void nnx_ctx_init(nnx_ctx *, void *, const nnx_adapter *);

#endif

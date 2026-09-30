#ifndef NNX_INTERNAL_H
#define NNX_INTERNAL_H

#include <stddef.h>
#include <nnx.h>

#define NNX_MAX_PARAMS 16
#define NNX_PARAM_NAME_MAX 64
#define NNX_PARAM_VALUE_MAX 256
#define NNX_MAX_VALUES 16
#define NNX_VALUE_KEY_MAX 64
#define NNX_MAX_GROUP_DEPTH 8

typedef enum nnx_method {
    NNX_GET, NNX_POST, NNX_PUT, NNX_PATCH,
    NNX_DELETE, NNX_HEAD, NNX_OPTIONS, NNX_METHOD_ANY
} nnx_method;

typedef struct nnx_middleware_entry {
    nnx_middleware_fn fn;
    void *data;
    void (*destroy)(void *data);
} nnx_middleware_entry;

struct nnx_group {
    nnx_app *app;
    struct nnx_group *parent;
    char *prefix;
    nnx_middleware_entry *middleware;
    size_t middleware_count;
    size_t middleware_capacity;
    struct nnx_group *next;
};

typedef struct nnx_route {
    nnx_method method;
    char *path;
    nnx_handler handler;
    nnx_group *group;
} nnx_route;

typedef struct nnx_param_pair {
    char name[NNX_PARAM_NAME_MAX];
    char value[NNX_PARAM_VALUE_MAX];
} nnx_param_pair;

typedef struct nnx_value_pair {
    char key[NNX_VALUE_KEY_MAX];
    void *value;
} nnx_value_pair;

typedef struct nnx_adapter {
    int (*send)(void *request, int status, const char *content_type,
                const void *body, size_t len);
    const char *(*query)(void *request, const char *name);
    const char *(*header)(void *request, const char *name);
    int (*set_header)(void *request, const char *name, const char *value);
    int (*log)(void *request, const char *message);
    void *(*alloc)(void *request, size_t size);
    const char *(*client_ip)(void *request);
    const char *(*scheme)(void *request);
} nnx_adapter;

struct nnx_app {
    nnx_route *routes;
    size_t route_count;
    size_t route_capacity;
    nnx_middleware_entry *middleware;
    size_t middleware_count;
    size_t middleware_capacity;
    nnx_group *groups;
    nnx_error_handler error_handler;
};

struct nnx_ctx {
    void *adapter_request;
    const nnx_adapter *adapter;
    int response_sent;
    int response_status;
    const char *method_name;
    const char *path;
    const unsigned char *body;
    size_t body_len;
    nnx_param_pair params[NNX_MAX_PARAMS];
    size_t param_count;
    char wildcard[NNX_PARAM_VALUE_MAX];
    const nnx_app *app;
    nnx_handler endpoint;
    size_t middleware_index;
    const nnx_group *group;
    const nnx_group *group_chain[NNX_MAX_GROUP_DEPTH];
    size_t group_depth;
    size_t group_index;
    size_t group_middleware_index;
    char request_id[64];
    int pending_error_status;
    nnx_value_pair values[NNX_MAX_VALUES];
    size_t value_count;
};

int nnx_add_route(nnx_app *, nnx_method, const char *, nnx_handler);
int nnx_add_group_route(nnx_group *, nnx_method, const char *, nnx_handler);
nnx_handler nnx_match_route(const nnx_app *, nnx_method, const char *, nnx_ctx *);
int nnx_route_path_exists(const nnx_app *, const char *);
void nnx_ctx_init(nnx_ctx *, void *, const nnx_adapter *,
                  const char *method_name, const char *path);
void nnx_ctx_set_body(nnx_ctx *, const void *body, size_t len);
void nnx_dispatch(nnx_app *, nnx_ctx *, nnx_handler);
void nnx_dispatch_error(nnx_app *, nnx_ctx *, int status);

#endif

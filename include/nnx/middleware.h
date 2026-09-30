#ifndef NNX_MIDDLEWARE_H
#define NNX_MIDDLEWARE_H

#include <nnx.h>

typedef struct nnx_cors_config {
    const char *allow_origin;
    const char *allow_methods;
    const char *allow_headers;
} nnx_cors_config;

typedef struct nnx_basic_auth_config {
    const char *username;
    const char *password;
    const char *realm;
} nnx_basic_auth_config;

typedef struct nnx_secure_config {
    const char *x_frame_options;
    const char *referrer_policy;
    const char *content_security_policy;
    int content_type_nosniff;
    long hsts_max_age;
    int hsts_include_subdomains;
    int hsts_preload;
} nnx_secure_config;

nnx_middleware nnx_logger(void);
nnx_middleware nnx_request_id_middleware(void);
nnx_middleware nnx_cors(nnx_cors_config config);
nnx_middleware nnx_basic_auth(nnx_basic_auth_config config);
nnx_middleware nnx_secure(nnx_secure_config config);

#endif

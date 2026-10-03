#ifndef NNX_TEST_NGX_CONFIG_H
#define NNX_TEST_NGX_CONFIG_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef unsigned char u_char;
typedef long ngx_int_t;

#define NGX_OK 0
#define NGX_ERROR -1
#define NGX_HTTP_HEAD 2
#define NGX_HTTP_NO_CONTENT 204
#define NGX_HTTP_NOT_MODIFIED 304
#define ngx_memcpy memcpy

#endif

#ifndef NNX_TEST_NGX_CORE_H
#define NNX_TEST_NGX_CORE_H

#include <ngx_config.h>

typedef struct ngx_pool_s ngx_pool_t;
typedef struct ngx_buf_s ngx_buf_t;

void *ngx_pnalloc(ngx_pool_t *pool, size_t size);
ngx_buf_t *ngx_calloc_buf(ngx_pool_t *pool);

#endif

#ifndef NNX_TEST_NGX_HTTP_H
#define NNX_TEST_NGX_HTTP_H

#include <sys/types.h>
#include <ngx_core.h>

typedef struct {
    size_t len;
    u_char *data;
} ngx_str_t;

typedef struct {
    unsigned int hash;
    ngx_str_t key;
    ngx_str_t value;
} ngx_table_elt_t;

typedef struct {
    ngx_table_elt_t entries[8];
    size_t count;
} ngx_list_t;

typedef struct {
    int status;
    off_t content_length_n;
    ngx_str_t content_type;
    ngx_list_t headers;
} ngx_http_headers_out_t;

typedef struct {
    ngx_pool_t *pool;
    ngx_http_headers_out_t headers_out;
    int method;
    int header_only;
    int header_sent;
} ngx_http_request_t;

struct ngx_buf_s {
    u_char *pos;
    u_char *last;
    unsigned int memory;
    unsigned int last_buf;
};

typedef struct ngx_chain_s {
    ngx_buf_t *buf;
    struct ngx_chain_s *next;
} ngx_chain_t;

void *ngx_list_push(ngx_list_t *list);
ngx_int_t ngx_http_send_header(ngx_http_request_t *request);
ngx_int_t ngx_http_output_filter(ngx_http_request_t *request, ngx_chain_t *chain);

#endif

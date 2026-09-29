#include <string.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

int nnx_nginx_send(void *request, int status, const char *body)
{
    ngx_http_request_t *r = request;
    ngx_buf_t *b; ngx_chain_t out; u_char *data; size_t len; ngx_int_t rc;
    if (!r || !body) return -1;
    len = strlen(body);
    r->headers_out.status = status;
    r->headers_out.content_length_n = (off_t)len;
    ngx_str_set(&r->headers_out.content_type, "text/plain");
    rc = ngx_http_send_header(r);
    if (rc == NGX_ERROR || rc > NGX_OK) return -1;
    if (r->header_only) return 0;
    data = ngx_pnalloc(r->pool, len ? len : 1);
    if (!data) return -1;
    if (len) ngx_memcpy(data, body, len);
    b = ngx_calloc_buf(r->pool);
    if (!b) return -1;
    b->pos = data; b->last = data + len;
    b->memory = 1; b->last_buf = 1;
    out.buf = b; out.next = NULL;
    rc = ngx_http_output_filter(r, &out);
    return rc == NGX_ERROR ? -1 : 0;
}

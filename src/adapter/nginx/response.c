#include <ngx_config.h>
#include <string.h>
#include <ngx_core.h>
#include <ngx_http.h>

int nnx_nginx_send(void *request, int status, const char *content_type,
                   const void *body, size_t len)
{
    ngx_http_request_t *r = request;
    ngx_buf_t *b;
    ngx_chain_t out;
    u_char *data;
    ngx_int_t rc;
    int send_body;

    if (!r || !content_type || (!body && len)) return -1;

    send_body = !r->header_only && r->method != NGX_HTTP_HEAD &&
                status != NGX_HTTP_NO_CONTENT &&
                status != NGX_HTTP_NOT_MODIFIED;
    if (send_body) {
        data = ngx_pnalloc(r->pool, len ? len : 1);
        if (!data) return -1;
        if (len) ngx_memcpy(data, body, len);

        b = ngx_calloc_buf(r->pool);
        if (!b) return -1;
        b->pos = data;
        b->last = data + len;
        b->memory = 1;
        b->last_buf = 1;
        out.buf = b;
        out.next = NULL;
    }

    r->headers_out.status = status;
    r->headers_out.content_length_n = (off_t)len;
    r->headers_out.content_type.len = strlen(content_type);
    r->headers_out.content_type.data = (u_char *)content_type;

    rc = ngx_http_send_header(r);
    if (rc == NGX_ERROR || rc > NGX_OK) return -1;
    if (!send_body || r->header_only) return 0;

    rc = ngx_http_output_filter(r, &out);
    return rc == NGX_ERROR ? -1 : 0;
}

int nnx_nginx_set_header(void *request, const char *name, const char *value)
{
    ngx_http_request_t *r = request;
    ngx_table_elt_t *h;
    u_char *key;
    u_char *header_value;
    size_t nl;
    size_t vl;

    if (!r || !name || !value) return -1;
    nl = strlen(name);
    vl = strlen(value);
    key = ngx_pnalloc(r->pool, nl ? nl : 1);
    if (!key) return -1;
    header_value = ngx_pnalloc(r->pool, vl ? vl : 1);
    if (!header_value) return -1;
    h = ngx_list_push(&r->headers_out.headers);
    if (!h) return -1;

    h->hash = 0;
    if (nl) ngx_memcpy(key, name, nl);
    if (vl) ngx_memcpy(header_value, value, vl);
    h->key.data = key;
    h->value.data = header_value;
    h->key.len = nl;
    h->value.len = vl;
    h->hash = 1;
    return 0;
}

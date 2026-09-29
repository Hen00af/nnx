#include <ngx_config.h>
#include <string.h>
#include <ngx_core.h>
#include <ngx_http.h>

static char *nnx_nginx_pool_string(ngx_pool_t *pool, const u_char *data, size_t len)
{
    char *s = ngx_pnalloc(pool, len + 1);
    if (!s) return NULL;
    if (len) ngx_memcpy(s, data, len);
    s[len] = 0;
    return s;
}

int nnx_nginx_send(void *request, int status, const char *content_type,
                   const void *body, size_t len)
{
    ngx_http_request_t *r = request;
    ngx_buf_t *b; ngx_chain_t out; u_char *data; ngx_int_t rc;
    if (!r || !content_type || (!body && len)) return -1;

    r->headers_out.status = status;
    r->headers_out.content_length_n = (off_t)len;
    r->headers_out.content_type.len = strlen(content_type);
    r->headers_out.content_type.data = (u_char *)content_type;

    rc = ngx_http_send_header(r);
    if (rc == NGX_ERROR || rc > NGX_OK) return -1;
    if (r->header_only) return 0;

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

    rc = ngx_http_output_filter(r, &out);
    return rc == NGX_ERROR ? -1 : 0;
}

const char *nnx_nginx_query(void *request, const char *name)
{
    ngx_http_request_t *r = request;
    ngx_str_t value;
    if (!r || !name) return NULL;
    if (ngx_http_arg(r, (u_char *)name, strlen(name), &value) != NGX_OK)
        return NULL;
    return nnx_nginx_pool_string(r->pool, value.data, value.len);
}

const char *nnx_nginx_header(void *request, const char *name)
{
    ngx_http_request_t *r = request;
    ngx_list_part_t *part;
    ngx_table_elt_t *headers;
    ngx_uint_t i;
    size_t n;

    if (!r || !name) return NULL;
    n = strlen(name);
    part = &r->headers_in.headers.part;

    for (;;) {
        headers = part->elts;
        for (i = 0; i < part->nelts; ++i) {
            if (headers[i].key.len == n &&
                ngx_strncasecmp(headers[i].key.data, (u_char *)name, n) == 0)
                return nnx_nginx_pool_string(r->pool, headers[i].value.data,
                                             headers[i].value.len);
        }
        if (!part->next) break;
        part = part->next;
    }
    return NULL;
}

int nnx_nginx_set_header(void *request, const char *name, const char *value)
{
    ngx_http_request_t *r = request;
    ngx_table_elt_t *h;
    size_t nl, vl;

    if (!r || !name || !value) return -1;
    nl = strlen(name);
    vl = strlen(value);
    h = ngx_list_push(&r->headers_out.headers);
    if (!h) return -1;
    h->hash = 1;
    h->key.data = ngx_pnalloc(r->pool, nl ? nl : 1);
    h->value.data = ngx_pnalloc(r->pool, vl ? vl : 1);
    if (!h->key.data || !h->value.data) return -1;
    if (nl) ngx_memcpy(h->key.data, name, nl);
    if (vl) ngx_memcpy(h->value.data, value, vl);
    h->key.len = nl;
    h->value.len = vl;
    return 0;
}

int nnx_nginx_log(void *request, const char *message)
{
    ngx_http_request_t *r = request;
    if (!r || !message) return -1;
    ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0, "%s", message);
    return 0;
}

#include <ngx_config.h>
#include <string.h>
#include <ngx_core.h>
#include <ngx_http.h>

static char *nnx_nginx_pool_string(ngx_pool_t *pool,
                                   const u_char *data, size_t len)
{
    char *s = ngx_pnalloc(pool, len + 1);
    if (!s) return NULL;
    if (len) ngx_memcpy(s, data, len);
    s[len] = 0;
    return s;
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

int nnx_nginx_log(void *request, const char *message)
{
    ngx_http_request_t *r = request;
    if (!r || !message) return -1;
    ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0, "%s", message);
    return 0;
}

void *nnx_nginx_alloc(void *request, size_t size)
{
    ngx_http_request_t *r = request;
    if (!r || !size) return NULL;
    return ngx_pnalloc(r->pool, size);
}

const char *nnx_nginx_client_ip(void *request)
{
    ngx_http_request_t *r = request;
    if (!r || !r->connection) return NULL;
    return nnx_nginx_pool_string(r->pool, r->connection->addr_text.data,
                                 r->connection->addr_text.len);
}

const char *nnx_nginx_scheme(void *request)
{
    ngx_http_request_t *r = request;
    if (!r || !r->connection) return NULL;
#if (NGX_HTTP_SSL)
    if (r->connection->ssl) return "https";
#endif
    return "http";
}

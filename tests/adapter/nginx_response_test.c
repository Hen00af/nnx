#include <assert.h>
#include <string.h>
#include <ngx_http.h>

struct ngx_pool_s {
    u_char bytes[4096];
    size_t used;
    int allocation_count;
    int fail_allocation;
    int fail_buffer;
    ngx_buf_t buffer;
};

static int header_calls;
static int body_calls;
static size_t body_length;

void *ngx_pnalloc(ngx_pool_t *pool, size_t size)
{
    void *out;
    ++pool->allocation_count;
    if (pool->allocation_count == pool->fail_allocation ||
        size > sizeof(pool->bytes) - pool->used)
        return NULL;
    out = pool->bytes + pool->used;
    pool->used += size;
    return out;
}

ngx_buf_t *ngx_calloc_buf(ngx_pool_t *pool)
{
    if (pool->fail_buffer) return NULL;
    memset(&pool->buffer, 0, sizeof(pool->buffer));
    return &pool->buffer;
}

void *ngx_list_push(ngx_list_t *list)
{
    if (list->count == 8) return NULL;
    return &list->entries[list->count++];
}

ngx_int_t ngx_http_send_header(ngx_http_request_t *request)
{
    size_t i;
    ++header_calls;
    if (request->header_sent) return NGX_ERROR;
    for (i = 0; i < request->headers_out.headers.count; ++i) {
        ngx_table_elt_t *h = &request->headers_out.headers.entries[i];
        if (h->hash)
            assert(h->key.data && h->value.data && h->key.len && h->value.len);
    }
    request->header_sent = 1;
    if (request->method == NGX_HTTP_HEAD ||
        request->headers_out.status == NGX_HTTP_NO_CONTENT ||
        request->headers_out.status == NGX_HTTP_NOT_MODIFIED)
        request->header_only = 1;
    return NGX_OK;
}

ngx_int_t ngx_http_output_filter(ngx_http_request_t *request, ngx_chain_t *chain)
{
    assert(request->header_sent);
    assert(chain && chain->buf && chain->buf->last_buf);
    ++body_calls;
    body_length = (size_t)(chain->buf->last - chain->buf->pos);
    return NGX_OK;
}

int nnx_nginx_send(void *, int, const char *, const void *, size_t);
int nnx_nginx_set_header(void *, const char *, const char *);

static void reset(ngx_http_request_t *request, ngx_pool_t *pool)
{
    memset(request, 0, sizeof(*request));
    memset(pool, 0, sizeof(*pool));
    request->pool = pool;
    header_calls = 0;
    body_calls = 0;
    body_length = 0;
}

int main(void)
{
    ngx_http_request_t request;
    ngx_pool_t pool;

    reset(&request, &pool);
    pool.fail_allocation = 1;
    assert(nnx_nginx_send(&request, 200, "text/plain", "ok", 2) == -1);
    assert(header_calls == 0 && request.header_sent == 0);
    pool.fail_allocation = 0;
    assert(nnx_nginx_send(&request, 500, "text/plain", "error", 5) == 0);
    assert(request.headers_out.status == 500 && body_length == 5);

    reset(&request, &pool);
    pool.fail_buffer = 1;
    assert(nnx_nginx_send(&request, 200, "text/plain", "ok", 2) == -1);
    assert(header_calls == 0 && request.header_sent == 0);

    reset(&request, &pool);
    request.method = NGX_HTTP_HEAD;
    pool.fail_allocation = 1;
    assert(nnx_nginx_send(&request, 200, "text/plain", "ok", 2) == 0);
    assert(header_calls == 1 && body_calls == 0);

    reset(&request, &pool);
    pool.fail_allocation = 1;
    assert(nnx_nginx_send(&request, 204, "text/plain", "", 0) == 0);
    assert(header_calls == 1 && body_calls == 0);

    reset(&request, &pool);
    pool.fail_allocation = 2;
    assert(nnx_nginx_set_header(&request, "X-Test", "value") == -1);
    assert(request.headers_out.headers.count == 0);
    pool.fail_allocation = 0;
    assert(nnx_nginx_send(&request, 500, "text/plain", "error", 5) == 0);

    reset(&request, &pool);
    assert(nnx_nginx_set_header(&request, "X-Test", "value") == 0);
    assert(request.headers_out.headers.count == 1);
    assert(nnx_nginx_send(&request, 200, "text/plain", "ok", 2) == 0);
    assert(body_calls == 1 && body_length == 2);
    return 0;
}

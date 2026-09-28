#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "nnx_internal.h"

/*
 * nnx deliberately does not vendor Nginx.
 * This file is compiled against headers from an external Nginx source/build.
 *
 * The module is intentionally thin: Nginx owns sockets, workers and its
 * event loop; nnx owns routing and the user-facing API.
 */

static ngx_int_t nnx_nginx_handler(ngx_http_request_t *r)
{
    static const char body[] = "nnx adapter is alive\n";
    ngx_buf_t *b;
    ngx_chain_t out;

    if (!(r->method & (NGX_HTTP_GET | NGX_HTTP_POST)))
        return NGX_HTTP_NOT_ALLOWED;

    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = sizeof(body) - 1;
    ngx_str_set(&r->headers_out.content_type, "text/plain");

    if (ngx_http_send_header(r) == NGX_ERROR)
        return NGX_ERROR;

    b = ngx_calloc_buf(r->pool);
    if (b == NULL)
        return NGX_HTTP_INTERNAL_SERVER_ERROR;

    b->pos = (u_char *)body;
    b->last = (u_char *)body + sizeof(body) - 1;
    b->memory = 1;
    b->last_buf = 1;

    out.buf = b;
    out.next = NULL;
    return ngx_http_output_filter(r, &out);
}

static char *nnx_enable(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t *clcf;

    (void)cmd;
    (void)conf;
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = nnx_nginx_handler;
    return NGX_CONF_OK;
}

static ngx_command_t nnx_commands[] = {
    {
        ngx_string("nnx"),
        NGX_HTTP_LOC_CONF | NGX_CONF_NOARGS,
        nnx_enable,
        0,
        0,
        NULL
    },
    ngx_null_command
};

static ngx_http_module_t nnx_module_ctx = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

ngx_module_t ngx_http_nnx_module = {
    NGX_MODULE_V1,
    &nnx_module_ctx,
    nnx_commands,
    NGX_HTTP_MODULE,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NGX_MODULE_V1_PADDING
};

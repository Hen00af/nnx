#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "../../internal/nnx_internal.h"

int nnx_nginx_send(void *request, int status, const char *content_type,
                   const void *body, size_t len);
const char *nnx_nginx_query(void *request, const char *name);
const char *nnx_nginx_header(void *request, const char *name);
int nnx_nginx_set_header(void *request, const char *name, const char *value);
int nnx_nginx_log(void *request, const char *message);
void *nnx_nginx_alloc(void *request, size_t size);
const char *nnx_nginx_client_ip(void *request);
const char *nnx_nginx_scheme(void *request);
ngx_int_t nnx_nginx_handle_request(ngx_http_request_t *r, nnx_app *app,
                                   const nnx_adapter *adapter);

static nnx_app *nnx_active_app;
static const nnx_adapter nnx_nginx_adapter = {
    nnx_nginx_send,
    nnx_nginx_query,
    nnx_nginx_header,
    nnx_nginx_set_header,
    nnx_nginx_log,
    nnx_nginx_alloc,
    nnx_nginx_client_ip,
    nnx_nginx_scheme
};

static ngx_int_t nnx_init_process(ngx_cycle_t *cycle)
{
    (void)cycle;
    nnx_active_app = nnx_new();
    if (!nnx_active_app) return NGX_ERROR;
    if (nnx_register(nnx_active_app) != 0) {
        nnx_free(nnx_active_app);
        nnx_active_app = NULL;
        return NGX_ERROR;
    }
    return NGX_OK;
}

static void nnx_exit_process(ngx_cycle_t *cycle)
{
    (void)cycle;
    nnx_free(nnx_active_app);
    nnx_active_app = NULL;
}

static ngx_int_t nnx_http_handler(ngx_http_request_t *r)
{
    return nnx_nginx_handle_request(r, nnx_active_app, &nnx_nginx_adapter);
}

static char *nnx_enable(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t *clcf;
    (void)cmd;
    (void)conf;
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = nnx_http_handler;
    return NGX_CONF_OK;
}

static ngx_command_t nnx_commands[] = {
    { ngx_string("nnx"), NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
      nnx_enable, 0, 0, NULL },
    ngx_null_command
};

static ngx_http_module_t nnx_module_ctx = {
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL
};

ngx_module_t ngx_http_nnx_module = {
    NGX_MODULE_V1, &nnx_module_ctx, nnx_commands, NGX_HTTP_MODULE,
    NULL,NULL,nnx_init_process,NULL,NULL,nnx_exit_process,NULL,
    NGX_MODULE_V1_PADDING
};

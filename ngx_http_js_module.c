#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#include <js/jsapi.h>

#include "ngx_http_js_module.h"
#include "nginx_js_glue.h"


static ngx_str_t  ngx_null_name = ngx_null_string;

static ngx_int_t
ngx_http_js_handler(ngx_http_request_t *r);


// callbacks

static char *
ngx_http_js_require(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	ngx_http_js_main_conf_t    *jsmcf = conf;
	u_char                    **p;
	ngx_str_t                  *value;
	
	value = cf->args->elts;
	
	p = ngx_array_push(&jsmcf->requires);
	
	if (p == NULL)
		return NGX_CONF_ERROR;
	
	*p = value[1].data;
	
	return NGX_CONF_OK;
}


static char *
ngx_http_js(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	ngx_http_js_loc_conf_t *jslcf = conf;

	ngx_str_t                  *value;
	ngx_http_core_loc_conf_t   *clcf;
	
	value = cf->args->elts;
	// fprintf(stderr, "js %s\n", value[1].data);
	if (jslcf->handler.data)
	{
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "duplicate js handler \"%V\"", &value[1]);
		return NGX_CONF_ERROR;
	}
	jslcf->handler = value[1];
	
	
	// JS side of question
	if (ngx_http_js__glue__set_callback(cf, cmd, jslcf) != NGX_CONF_OK)
		return NGX_CONF_ERROR;
	
	
	clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
	clcf->handler = ngx_http_js_handler;
	
	return NGX_CONF_OK;
}


static char *
ngx_http_js_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_js_handler(ngx_http_request_t *r)
{
	if (r->zero_in_uri)
		return NGX_HTTP_NOT_FOUND;
	
	JSObject                   *sub;
	ngx_int_t                   rc;
	ngx_str_t                   uri, args, *handler;
	ngx_http_js_ctx_t          *ctx;
	ngx_http_js_loc_conf_t     *jslcf;
	ngx_http_js_main_conf_t    *jsmcf;
	
	ctx = ngx_http_get_module_ctx(r, ngx_http_js_module);
	
	if (ctx == NULL)
	{
		ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_js_ctx_t));
		if (ctx == NULL)
		{
			ngx_http_finalize_request(r, NGX_ERROR);
			return NGX_DONE;
		}
		
		ngx_http_set_ctx(r, ctx, ngx_http_js_module);
	}

    jsmcf = ngx_http_get_module_main_conf(r, ngx_http_js_module);

    {

    if (ctx->next == NULL) {
        jslcf = ngx_http_get_module_loc_conf(r, ngx_http_js_module);
        sub = jslcf->sub;
        handler = &jslcf->handler;

    } else {
        sub = ctx->next;
        handler = &ngx_null_name;
        ctx->next = NULL;
    }
	
	rc = ngx_http_js__glue__call_handler(jsmcf->js_cx, jsmcf->js_global, r, sub, handler);

    }

    if (rc == NGX_DONE) {
        return NGX_DONE;
    }

    if (rc > 600) {
        rc = NGX_OK;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "js handler done: %i", rc);

    if (ctx->redirect_uri.len) {
        uri = ctx->redirect_uri;
        args = ctx->redirect_args;

    } else {
        uri.len = 0;
    }

    ctx->filename.data = NULL;
    ctx->redirect_uri.len = 0;

    if (ctx->done || ctx->next) {
        return NGX_DONE;
    }

    if (uri.len) {
        ngx_http_internal_redirect(r, &uri, &args);
        return NGX_DONE;
    }

    if (rc == NGX_OK || rc == NGX_HTTP_OK) {
        ngx_http_send_special(r, NGX_HTTP_LAST);
        ctx->done = 1;
    }

    ngx_http_finalize_request(r, rc);
	
	return NGX_DONE;
}


// configuration

static void *
ngx_http_js_create_main_conf(ngx_conf_t *cf)
{
    ngx_http_js_main_conf_t  *jsmcf;

    jsmcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_js_main_conf_t));
    if (jsmcf == NULL) {
        return NGX_CONF_ERROR;
    }

    if (ngx_array_init(&jsmcf->requires, cf->pool, 1, sizeof(u_char *))
        != NGX_OK)
    {
        return NULL;
    }

    return jsmcf;
}


static char *
ngx_http_js_init_main_conf(ngx_conf_t *cf, void *conf)
{
	return ngx_http_js__glue__init_interpreter(cf, (ngx_http_js_main_conf_t*)conf);
}


static void *
ngx_http_js_create_loc_conf(ngx_conf_t *cf)
{
	ngx_http_js_loc_conf_t *jslcf;
	
	jslcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_js_loc_conf_t));
	if (jslcf == NULL)
		return NGX_CONF_ERROR;

	// set by ngx_pcalloc():
	//  jslcf->handler = { 0, NULL };

	return jslcf;
}




static ngx_command_t  ngx_http_js_commands[] = {

    { ngx_string("js_require"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
      ngx_http_js_require,
      NGX_HTTP_MAIN_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("js"),
      NGX_HTTP_LOC_CONF|NGX_HTTP_LMT_CONF|NGX_CONF_TAKE1,
      ngx_http_js,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("js_set"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE2,
      ngx_http_js_set,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

      ngx_null_command
};

static ngx_http_module_t  ngx_http_js_module_ctx = {
    NULL,                                  /* preconfiguration */
    NULL,                                  /* postconfiguration */

    ngx_http_js_create_main_conf,          /* create main configuration */
    ngx_http_js_init_main_conf,            /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    ngx_http_js_create_loc_conf,           /* create location configuration */
    NULL                                   /* merge location configuration */
};


ngx_module_t  ngx_http_js_module = {
    NGX_MODULE_V1,
    &ngx_http_js_module_ctx,               /* module context */
    ngx_http_js_commands,                  /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


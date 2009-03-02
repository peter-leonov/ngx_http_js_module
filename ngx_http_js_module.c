#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

// #include <js/jsapi.h>

#include "ngx_http_js_module.h"
#include "nginx_js_glue.h"

#include "macroses.h"

// static ngx_str_t  ngx_null_name = ngx_null_string;

static ngx_int_t
ngx_http_js_handler(ngx_http_request_t *r);


// callbacks

static char *
ngx_http_js_require(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	ngx_http_js_main_conf_t    *jsmcf;
	u_char                    **p;
	ngx_str_t                  *value;
	
	TRACE();
	
	value = cf->args->elts;
	
	jsmcf = conf;
	p = ngx_array_push(&jsmcf->requires);
	
	if (p == NULL)
		return NGX_CONF_ERROR;
	
	*p = value[1].data;
	
	return NGX_CONF_OK;
}


static char *
ngx_http_js(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	ngx_http_js_loc_conf_t     *jslcf;
	ngx_str_t                  *value;
	ngx_http_core_loc_conf_t   *clcf;
	
	TRACE();
	
	jslcf = conf;
	value = cf->args->elts;
	// LOG("js %s\n", value[1].data);
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
	TRACE();
	return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_js_handler(ngx_http_request_t *r)
{
	TRACE();
	return ngx_http_js__glue__call_handler(r);
}


// configuration

static void *
ngx_http_js_create_main_conf(ngx_conf_t *cf)
{
	ngx_http_js_main_conf_t  *jsmcf;
	
	TRACE();
	
	jsmcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_js_main_conf_t));
	if (jsmcf == NULL)
		return NGX_CONF_ERROR;
	
	if (ngx_array_init(&jsmcf->requires, cf->pool, 1, sizeof(u_char *)) != NGX_OK)
		return NULL;
	
	return jsmcf;
}


static char *
ngx_http_js_init_main_conf(ngx_conf_t *cf, void *conf)
{
	TRACE();
	return ngx_http_js__glue__init_interpreter(cf, (ngx_http_js_main_conf_t*)conf);
}


static void *
ngx_http_js_create_loc_conf(ngx_conf_t *cf)
{
	ngx_http_js_loc_conf_t *jslcf;
	
	TRACE();
	
	jslcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_js_loc_conf_t));
	if (jslcf == NULL)
		return NGX_CONF_ERROR;
	
	// set by ngx_pcalloc():
	//  jslcf->handler = { 0, NULL };

	return jslcf;
}

static ngx_int_t
ngx_http_js_filter_init(ngx_conf_t *cf)
{
	TRACE();
    // ngx_http_next_header_filter = ngx_http_top_header_filter;
    // ngx_http_top_header_filter = ngx_http_addition_header_filter;
    // 
    // ngx_http_next_body_filter = ngx_http_top_body_filter;
    // ngx_http_top_body_filter = ngx_http_addition_body_filter;

    return NGX_OK;
}



static ngx_command_t  ngx_http_js_commands[] =
{
	{
		ngx_string("js_require"),
		NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
		ngx_http_js_require,
		NGX_HTTP_MAIN_CONF_OFFSET,
		0,
		NULL
	},

	{
		ngx_string("js"),
		NGX_HTTP_LOC_CONF|NGX_HTTP_LMT_CONF|NGX_CONF_TAKE1,
		ngx_http_js,
		NGX_HTTP_LOC_CONF_OFFSET,
		0,
		NULL
	},
	
	{
		ngx_string("js_set"),
		NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE2,
		ngx_http_js_set,
		NGX_HTTP_LOC_CONF_OFFSET,
		0,
		NULL
	},
	
	ngx_null_command
};

static ngx_http_module_t  ngx_http_js_module_ctx =
{
	NULL,                                  /* preconfiguration */
	ngx_http_js_filter_init,               /* postconfiguration */
	
	ngx_http_js_create_main_conf,          /* create main configuration */
	ngx_http_js_init_main_conf,            /* init main configuration */
	
	NULL,                                  /* create server configuration */
	NULL,                                  /* merge server configuration */
	
	ngx_http_js_create_loc_conf,           /* create location configuration */
	NULL                                   /* merge location configuration */
};


ngx_module_t  ngx_http_js_module =
{
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

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#include <js/jsapi.h>

#include "ngx_http_js_module.h"
#include "nginx_js_glue.h"

#include "macroses.h"

// static ngx_str_t  ngx_null_name = ngx_null_string;

static ngx_int_t
ngx_http_js_handler(ngx_http_request_t *r);


ngx_http_output_header_filter_pt  ngx_http_js_next_header_filter = NULL;
ngx_http_output_body_filter_pt    ngx_http_js_next_body_filter = NULL;

// callbacks

static char *
ngx_http_js_load(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	ngx_http_js_main_conf_t    *jsmcf;
	u_char                    **p;
	ngx_str_t                  *value;
	
	TRACE();
	
	value = cf->args->elts;
	
	jsmcf = conf;
	p = ngx_array_push(&jsmcf->loads);
	
	if (p == NULL)
		return NGX_CONF_ERROR;
	
	*p = value[1].data;
	
	return NGX_CONF_OK;
}

static char *
ngx_http_js_utf8(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	JS_SetCStringsAreUTF8();
	
	if (JS_CStringsAreUTF8())
		return NGX_CONF_OK;
	else
	{
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "couldn't set JS_CStringsAreUTF8()", &cmd->name);
		return NGX_CONF_ERROR;
	}
}


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
	if (jslcf->handler_name.data)
	{
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "duplicate js handler \"%V\"", &value[1]);
		return NGX_CONF_ERROR;
	}
	jslcf->handler_name = value[1];
	
	
	// JS side of question
	if (ngx_http_js__glue__set_callback(cf, cmd, jslcf) != NGX_CONF_OK)
		return NGX_CONF_ERROR;
	
	
	clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
	clcf->handler = ngx_http_js_handler;
	
	return NGX_CONF_OK;
}


static char *
ngx_http_js_filter(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	ngx_http_js_loc_conf_t     *jslcf;
	ngx_str_t                  *value;
	
	TRACE();
	
	jslcf = conf;
	value = cf->args->elts;
	// LOG("js %s\n", value[1].data);
	if (jslcf->filter_name.data)
	{
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "duplicate js filter \"%V\"", &value[1]);
		return NGX_CONF_ERROR;
	}
	jslcf->filter_name = value[1];
	
	
	// JS side of question
	if (ngx_http_js__glue__set_filter(cf, cmd, jslcf) != NGX_CONF_OK)
		return NGX_CONF_ERROR;
	
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
	if (r->zero_in_uri)
		return NGX_HTTP_NOT_FOUND;
	
	r->main->count++;
	
	ngx_http_js__glue__call_handler(r);
	
	// return implies ngx_http_finalize_request() which in turn implies count--
	return NGX_DONE;
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
	
	if (ngx_array_init(&jsmcf->loads, cf->pool, 1, sizeof(u_char *)) != NGX_OK)
		return NULL;
	
	jsmcf->maxmem = NGX_CONF_UNSET_SIZE;
	
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
	TRACE();
	
	ngx_http_js_loc_conf_t *jslcf;
	
	
	jslcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_js_loc_conf_t));
	if (jslcf == NULL)
		return NGX_CONF_ERROR;
	
	// set by ngx_pcalloc():
	//  jslcf->handler_function = NULL;

	return jslcf;
}


static char *
ngx_http_js_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
	ngx_http_js_loc_conf_t *prev = parent;
	ngx_http_js_loc_conf_t *conf = child;
	
	if (conf->filter_function == NULL)
		conf->filter_function = prev->filter_function;
	
	if (conf->filter_types == NULL)
	{
		if (prev->filter_types == NULL)
		{
			conf->filter_types = ngx_array_create(cf->pool, 1, sizeof(ngx_str_t));
			if (conf->filter_types == NULL)
				return NGX_CONF_ERROR;
		}
		else
			conf->filter_types = prev->filter_types;
	}
	
	return NGX_CONF_OK;
}

/*
static ngx_int_t
ngx_http_js_header_filter(ngx_http_request_t *r)
{
	ngx_uint_t                i;
	ngx_str_t                *type;
	ngx_http_js_loc_conf_t   *jslcf;
	ngx_http_js_ctx_t        *ctx;
	
	TRACE();
	
	jslcf = ngx_http_get_module_loc_conf(r, ngx_http_js_module);
	
	if
	(
		!jslcf->filter_function
		|| r->headers_out.content_type.len == 0
		|| r->headers_out.content_length_n == 0
	)
		return ngx_http_js_next_header_filter(r);
	
	
	type = jslcf->filter_types->elts;
	for (i = 0; i < jslcf->filter_types->nelts; i++)
	{
		if
		(
			r->headers_out.content_type.len >= type[i].len
			&& ngx_strncasecmp(r->headers_out.content_type.data, type[i].data, type[i].len) == 0
		)
			goto found;
	}
	
	return ngx_http_js_next_header_filter(r);
	
	
	found:
	
	// LOG("found");
	
	if (!(ctx = ngx_http_get_module_ctx(r, ngx_http_js_module)))
	{
		// ngx_pcalloc fills allocated memory with zeroes
		ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_js_ctx_t));
		if (ctx == NULL)
			return NGX_ERROR;
		
		ngx_http_set_ctx(r, ctx, ngx_http_js_module);
	}
	
	ctx->filter_enabled = 1;
	
	r->filter_need_in_memory = 1;
	
	if (r == r->main)
	{
		ngx_http_clear_content_length(r);
		ngx_http_clear_last_modified(r);
	}
	
	return ngx_http_js_next_header_filter(r);
}

static ngx_int_t
ngx_http_js_body_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
	ngx_http_js_ctx_t        *ctx;
	
	ngx_log_debug3(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http js filter \"%V?%V\" %p", &r->uri, &r->args, in);
	
	if ((ctx = ngx_http_get_module_ctx(r, ngx_http_js_module)) && ctx->filter_enabled)
		return ngx_http_js__glue__call_filter(r, in);
	
	return ngx_http_js_next_body_filter(r, in);
}
*/

static ngx_int_t
ngx_http_js_header_buffer_filter(ngx_http_request_t *r)
{
	ngx_http_js_ctx_t        *ctx;
	
	if (r == r->main)
		return ngx_http_js_next_header_filter(r);
	
	if (!(ctx = ngx_http_get_module_ctx(r, ngx_http_js_module)))
	{
		// ngx_pcalloc fills allocated memory with zeroes
		ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_js_ctx_t));
		if (ctx == NULL)
			return NGX_ERROR;
		
		ngx_http_set_ctx(r, ctx, ngx_http_js_module);
	}
	
	ctx->filter_enabled = 1;
	ctx->chain_first = NULL;
	ctx->chain_last = NULL;
	
	r->filter_need_in_memory = 1;
	
	
	return ngx_http_js_next_header_filter(r);
}

static ngx_int_t
ngx_http_js_body_buffer_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
	ngx_http_js_ctx_t        *ctx;
	
	ngx_log_debug3(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http js buffer filter \"%V?%V\" %p", &r->uri, &r->args, in);
	
	if ((ctx = ngx_http_get_module_ctx(r, ngx_http_js_module)) && ctx->filter_enabled)
	{
		ngx_chain_t *cl;
		
		for (; in != NULL; in = in->next)
		{
			ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "buf: %*s", in->buf->last - in->buf->pos, in->buf->pos);
			
			cl = ngx_palloc(r->pool, sizeof(ngx_chain_t));
			if (cl == NULL)
				return NGX_ERROR;
			
			cl->buf = in->buf;
			
			if (ctx->chain_last)
			{
				ctx->chain_last->next = cl;
				ctx->chain_last = cl;
			}
			else
			{
				ctx->chain_first = cl;
				ctx->chain_last = cl;
			}
			
			// if (chain_link->buf->last_buf)
		}
		ctx->chain_last->next = NULL;
		
		return NGX_OK;
	}
	
	return ngx_http_js_next_body_filter(r, in);
}

static ngx_int_t
ngx_http_js_filter_init(ngx_conf_t *cf)
{
	TRACE();
	
	ngx_http_js_next_header_filter = ngx_http_top_header_filter;
	// ngx_http_top_header_filter = ngx_http_js_header_filter;
	ngx_http_top_header_filter = ngx_http_js_header_buffer_filter;
	
	ngx_http_js_next_body_filter = ngx_http_top_body_filter;
	// ngx_http_top_body_filter = ngx_http_js_body_filter;
	ngx_http_top_body_filter = ngx_http_js_body_buffer_filter;
	
	return NGX_OK;
}


static char *
ngx_http_js_filter_types(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	TRACE();
	
	ngx_http_js_loc_conf_t *jslcf = conf;
	
	ngx_str_t   *value, *type;
	ngx_uint_t   i;
	
	if (cf->args->nelts == 0)
	{
		jslcf->filter_types = NULL;
		return NGX_CONF_OK;
	}
	
	if (jslcf->filter_types == NULL)
	{
		jslcf->filter_types = ngx_array_create(cf->pool, 4, sizeof(ngx_str_t));
		if (jslcf->filter_types == NULL)
			return NGX_CONF_ERROR;
	}
	
	value = cf->args->elts;
	
	for (i = 1; i < cf->args->nelts; i++)
	{
		type = ngx_array_push(jslcf->filter_types);
		if (type == NULL)
			return NGX_CONF_ERROR;
		
		type->len = value[i].len;
		
		type->data = ngx_palloc(cf->pool, type->len + 1);
		if (type->data == NULL)
			return NGX_CONF_ERROR;
		
		ngx_cpystrn(type->data, value[i].data, type->len + 1);
    }

    return NGX_CONF_OK;
}


static ngx_command_t  ngx_http_js_commands[] =
{
	{
		ngx_string("js_maxmem"),
		NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
		ngx_conf_set_size_slot,
		NGX_HTTP_MAIN_CONF_OFFSET,
		offsetof(ngx_http_js_main_conf_t, maxmem),
		NULL
	},
	
	{
		ngx_string("js_utf8"),
		NGX_HTTP_MAIN_CONF|NGX_CONF_NOARGS,
		ngx_http_js_utf8,
		NGX_HTTP_MAIN_CONF_OFFSET,
		0,
		NULL
	},
	
	{
		ngx_string("js_load"),
		NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
		ngx_http_js_load,
		NGX_HTTP_MAIN_CONF_OFFSET,
		0,
		NULL
	},
	
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
		ngx_string("js_filter"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
		ngx_http_js_filter,
		NGX_HTTP_LOC_CONF_OFFSET,
		0,
		NULL
	},
	
	{
		ngx_string("js_filter_types"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
		ngx_http_js_filter_types,
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
	ngx_http_js_merge_loc_conf             /* merge location configuration */
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

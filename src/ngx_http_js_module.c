#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#include <ngx_http_jsapi.h>

#include <ngx_http_js_module.h>
#include <nginx_js_glue.h>

#include <nginx_js_macroses.h>

static ngx_int_t
ngx_http_js_content_handler(ngx_http_request_t *r);

static ngx_int_t access_phase_needed;

ngx_http_output_header_filter_pt  ngx_http_js_next_header_filter = NULL;
ngx_http_output_body_filter_pt    ngx_http_js_next_body_filter = NULL;


static char *
command__js_load(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	ngx_http_js_main_conf_t    *jsmcf;
	u_char                    **p;
	ngx_str_t                  *value;
	
	TRACE();
	
	value = cf->args->elts;
	
	jsmcf = conf;
	p = ngx_array_push(&jsmcf->loads);
	if (p == NULL)
	{
		return NGX_CONF_ERROR;
	}
	
	*p = value[1].data;
	
	return NGX_CONF_OK;
}

#if (NGX_HTTP_JS_SET_STRINGS_ARE_UTF8)
static char *
command__js_utf8(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	JS_SetCStringsAreUTF8();
	
	if (JS_CStringsAreUTF8())
	{
		return NGX_CONF_OK;
	}
	else
	{
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "couldn't set JS_CStringsAreUTF8()", &cmd->name);
		return NGX_CONF_ERROR;
	}
}
#endif


static char *
command__js_require(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	ngx_http_js_main_conf_t    *jsmcf;
	u_char                    **p;
	ngx_str_t                  *value;
	
	TRACE();
	
	value = cf->args->elts;
	
	jsmcf = conf;
	p = ngx_array_push(&jsmcf->requires);
	
	if (p == NULL)
	{
		return NGX_CONF_ERROR;
	}
	
	*p = value[1].data;
	
	return NGX_CONF_OK;
}


static char *
command__js_access(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	ngx_http_js_loc_conf_t     *jslcf;
	ngx_str_t                  *value;
	
	TRACE();
	
	jslcf = conf;
	value = cf->args->elts;
	
	if (jslcf->access_handler_name.data)
	{
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "duplicate js access handler \"%V\"", &value[1]);
		return NGX_CONF_ERROR;
	}
	jslcf->access_handler_name = value[1];
	
	
	// JS side of question
	if (ngx_http_js__glue__set_handler(cf, &jslcf->access_handler_function, "js access handler") != NGX_CONF_OK)
	{
		return NGX_CONF_ERROR;
	}
	
	access_phase_needed = 1;
	
	return NGX_CONF_OK;
}


static char *
command__js(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	ngx_http_js_loc_conf_t     *jslcf;
	ngx_str_t                  *value;
	ngx_http_core_loc_conf_t   *clcf;
	
	TRACE();
	
	jslcf = conf;
	value = cf->args->elts;
	
	if (jslcf->content_handler_name.data)
	{
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "duplicate js content handler \"%V\"", &value[1]);
		return NGX_CONF_ERROR;
	}
	jslcf->content_handler_name = value[1];
	
	
	// JS side of question
	if (ngx_http_js__glue__set_handler(cf, &jslcf->content_handler_function, "js content handler") != NGX_CONF_OK)
	{
		return NGX_CONF_ERROR;
	}
	
	clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
	clcf->handler = ngx_http_js_content_handler;
	
	return NGX_CONF_OK;
}


static char *
command__js_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	return ngx_http_js__glue__js_set(cf, cmd, conf);
}


static ngx_int_t
ngx_http_js_content_handler(ngx_http_request_t *r)
{
#if defined(nginx_version) && (nginx_version >= 8011)
	r->main->count++;
#endif
	
	ngx_http_finalize_request(r, ngx_http_js__glue__content_handler(r));
	
	// return implies ngx_http_finalize_request()
	// which in turn implies count--
	// for the first count = 1 from ngx_http_init_request()
	return NGX_DONE;
}


static void *
ngx_http_js_create_main_conf(ngx_conf_t *cf)
{
	ngx_http_js_main_conf_t  *jsmcf;
	
	TRACE();
	
	jsmcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_js_main_conf_t));
	if (jsmcf == NULL)
	{
		return NGX_CONF_ERROR;
	}
	
	if (ngx_array_init(&jsmcf->requires, cf->pool, 1, sizeof(u_char *)) != NGX_OK)
	{
		return NULL;
	}
	
	if (ngx_array_init(&jsmcf->loads, cf->pool, 1, sizeof(u_char *)) != NGX_OK)
	{
		return NULL;
	}
	
	jsmcf->maxmem = NGX_CONF_UNSET_SIZE;
	
	return jsmcf;
}


static char *
ngx_http_js_init_main_conf(ngx_conf_t *cf, void *conf)
{
	TRACE();
	return ngx_http_js__glue__init_interpreter(cf);
}


static void *
ngx_http_js_create_loc_conf(ngx_conf_t *cf)
{
	TRACE();
	
	ngx_http_js_loc_conf_t *jslcf;
	
	
	jslcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_js_loc_conf_t));
	if (jslcf == NULL)
	{
		return NGX_CONF_ERROR;
	}
	
	// set by ngx_pcalloc():
	//  jslcf->content_handler_function = NULL;

	return jslcf;
}

static ngx_int_t
ngx_http_js_init_worker(ngx_cycle_t *cycle)
{
	TRACE();
	return ngx_http_js__glue__init_worker(cycle);
}

static void
ngx_http_js_exit_worker(ngx_cycle_t *cycle)
{
	TRACE();
	ngx_http_js__glue__exit_worker(cycle);
}

static void
ngx_http_js_exit_master(ngx_cycle_t *cycle)
{
	TRACE();
	ngx_http_js__glue__exit_master(cycle);
}

static char *
ngx_http_js_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
	return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_js_body_buffer_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
	ngx_http_js_ctx_t        *ctx;
	ngx_chain_t              *cl;
	
	ngx_log_debug3(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http js buffer filter \"%V?%V\" %p", &r->uri, &r->args, in);
	
	ctx = ngx_http_get_module_ctx(r, ngx_http_js_module);
	if (ctx == NULL)
	{
		return ngx_http_js_next_body_filter(r, in);
	}
	
	if (!ctx->filter_enabled)
	{
		return ngx_http_js_next_body_filter(r, in);
	}
	
	
	for (; in != NULL; in = in->next)
	{
		ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "buf: %*s", in->buf->last - in->buf->pos, in->buf->pos);
		
		cl = ngx_palloc(r->pool, sizeof(ngx_chain_t));
		if (cl == NULL)
		{
			return NGX_ERROR;
		}
		
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

static ngx_int_t
init_access_phase(ngx_conf_t *cf)
{
	ngx_http_handler_pt        *h;
	ngx_http_core_main_conf_t  *cmcf;
	
	if (!access_phase_needed)
	{
		return NGX_OK;
	}
	
	cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);
	
	h = ngx_array_push(&cmcf->phases[NGX_HTTP_ACCESS_PHASE].handlers);
	if (h == NULL)
	{
		return NGX_ERROR;
	}
	
	*h = ngx_http_js__glue__access_handler;
	
	return NGX_OK;
}

static ngx_int_t
postconfiguration(ngx_conf_t *cf)
{
	ngx_int_t     rc;
	TRACE();
	
	rc = init_access_phase(cf);
	if (rc != NGX_OK)
	{
		return rc;
	}
	
	ngx_http_js_next_body_filter = ngx_http_top_body_filter;
	ngx_http_top_body_filter = ngx_http_js_body_buffer_filter;
	
	return NGX_OK;
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
	
#if (NGX_HTTP_JS_SET_STRINGS_ARE_UTF8)
	{
		ngx_string("js_utf8"),
		NGX_HTTP_MAIN_CONF|NGX_CONF_NOARGS,
		command__js_utf8,
		NGX_HTTP_MAIN_CONF_OFFSET,
		0,
		NULL
	},
#endif
	
	{
		ngx_string("js_load"),
		NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
		command__js_load,
		NGX_HTTP_MAIN_CONF_OFFSET,
		0,
		NULL
	},
	
	{
		ngx_string("js_require"),
		NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
		command__js_require,
		NGX_HTTP_MAIN_CONF_OFFSET,
		0,
		NULL
	},
	
	{
		ngx_string("js"),
		NGX_HTTP_LOC_CONF|NGX_HTTP_LMT_CONF|NGX_CONF_TAKE1,
		command__js,
		NGX_HTTP_LOC_CONF_OFFSET,
		0,
		NULL
	},
	
	{
		ngx_string("js_access"),
		NGX_HTTP_LOC_CONF|NGX_HTTP_LMT_CONF|NGX_CONF_TAKE1,
		command__js_access,
		NGX_HTTP_LOC_CONF_OFFSET,
		0,
		NULL
	},
	
	{
		ngx_string("js_set"),
		NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE2,
		command__js_set,
		NGX_HTTP_LOC_CONF_OFFSET,
		0,
		NULL
	},
	
	ngx_null_command
};

static ngx_http_module_t  ngx_http_js_module_ctx =
{
	NULL,                                  /* preconfiguration */
	postconfiguration,                     /* postconfiguration */
	
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
	ngx_http_js_init_worker,               /* init process */
	NULL,                                  /* init thread */
	NULL,                                  /* exit thread */
	ngx_http_js_exit_worker,               /* exit process */
	ngx_http_js_exit_master,               /* exit master */
	NGX_MODULE_V1_PADDING
};

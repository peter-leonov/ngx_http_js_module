#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#include <js/jsapi.h>

#include "ngx_http_js_module.h"
#include "nginx_js_glue.h"
#include "classes/global.h"
#include "classes/Nginx.h"
#include "classes/Request.h"
#include "classes/HeadersIn.h"
#include "classes/HeadersOut.h"
#include "classes/Chain.h"
#include "classes/File.h"

#include "macroses.h"

JSRuntime *ngx_http_js_module_js_runtime = NULL;
JSContext *ngx_http_js_module_js_context = NULL;
JSObject  *ngx_http_js_module_js_global  = NULL;


ngx_log_t *ngx_http_js_module_log = NULL;

static void
reportError(JSContext *cx, const char *message, JSErrorReport *report)
{
	TRACE();
	
	ngx_log_error
	(
		NGX_LOG_ERR, ngx_http_js_module_log ? ngx_http_js_module_log : ngx_cycle->log, 0,
		"%s%s%s at %s:%i",
		COLOR_RED, message, COLOR_CLEAR,
		report->filename ? report->filename : "<no filename>",
		(ngx_uint_t) report->lineno
	);
}


static JSBool
ngx_http_js_load(JSContext *cx, JSObject *global, char *filename)
{
	jsval           fval, rval, strval;
	JSString       *fnstring;
	// JSObject       *require;
	
	TRACE();
	// LOG("ngx_http_js_load(%s)\n", filename);
	
	if (!JS_GetProperty(cx, global, "load", &fval))
	{
		JS_ReportError(cx, "global.load is undefined");
		return JS_FALSE;
	}
	if (!JSVAL_IS_OBJECT(fval) || !JS_ValueToFunction(cx, fval))
	{
		JS_ReportError(cx, "global.load is not a function");
		return JS_FALSE;
	}
	
	fnstring = JS_NewStringCopyZ(cx, filename);
	if (!fnstring)
		return JS_FALSE;
		
	strval = STRING_TO_JSVAL(fnstring);
	
	if (!JS_CallFunctionValue(cx, global, fval, 1, &strval, &rval))
	{
		JS_ReportError(cx, "error calling global.load from nginx");
		return JS_FALSE;
	}
	
	return JS_TRUE;
}

static ngx_int_t
ngx_http_js_run_loads(JSContext *cx, JSObject *global, ngx_array_t *requires, ngx_log_t *log)
{
	char          **script;
	ngx_uint_t      i;
	char           *value;
	
	TRACE();
		
	script = requires->elts;
	for (i = 0; i < requires->nelts; i++)
	{
		value = script[i];
		ngx_log_debug1(NGX_LOG_DEBUG_HTTP, log, 0, "js_load: %s\n", value);
		
		if (!ngx_http_js_load(cx, global, (char*)value))
			return NGX_ERROR;
	}
	
	return NGX_OK;
}


static ngx_int_t
ngx_http_js_run_requires(JSContext *cx, JSObject *global, ngx_array_t *requires, ngx_log_t *log)
{
	char          **script;
	ngx_uint_t      i;
	jsval           rval, strval, fval;
	char           *value;
	
	TRACE();
	
	if (requires->nelts == 0)
		return NGX_OK;
	
	if (!JS_GetProperty(cx, global, "require", &fval))
	{
		JS_ReportError(cx, "global.require is undefined");
		return NGX_ERROR;
	}
	if (!JSVAL_IS_OBJECT(fval) || !JS_ValueToFunction(cx, fval))
	{
		JS_ReportError(cx, "global.require is not a Function object");
		return NGX_ERROR;
	}
	
	script = requires->elts;
	for (i = 0; i < requires->nelts; i++)
	{
		value = script[i];
		ngx_log_debug1(NGX_LOG_DEBUG_HTTP, log, 0, "js_require: %s\n", value);
		
		strval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, (char*)value));
		if (!JS_CallFunctionValue(cx, global, fval, 1, &strval, &rval))
		{
			JS_ReportError(cx, "error calling global.require from nginx");
			return NGX_ERROR;
		}
	}
	
	return NGX_OK;
}



char *
ngx_http_js__glue__init_interpreter(ngx_conf_t *cf, ngx_http_js_main_conf_t *jsmcf)
{
	if (ngx_http_js_module_js_runtime != NULL)
		return NGX_CONF_OK;
	
	JSContext   *cx;
	JSRuntime   *rt;
	JSObject    *global;
	
	ngx_log_debug0(NGX_LOG_DEBUG, cf->log, 0, "init interpreter");
	
	if (ngx_set_environment(cf->cycle, NULL) == NULL)
		return NGX_CONF_ERROR;
	
	
	rt = JS_NewRuntime(jsmcf->maxmem == NGX_CONF_UNSET_SIZE ? 2L * 1024L * 1024L : jsmcf->maxmem);
	if (rt == NULL)
		return NGX_CONF_ERROR;
	
	cx = JS_NewContext(rt, 8192);
	if (cx == NULL)
		return NGX_CONF_ERROR;
	
	JS_SetOptions(cx, JSOPTION_VAROBJFIX);
	JS_SetVersion(cx, 170);
	JS_SetErrorReporter(cx, reportError);
	
	
	// global
	if (!ngx_http_js__global__init(cx))
	{
		ngx_conf_log_error(NGX_LOG_EMERG, cf, ngx_errno, "global object initialization failed");
		return NGX_CONF_ERROR;
	}
	global = JS_GetGlobalObject(cx);
	
	// Nginx
	if (!ngx_http_js__nginx__init(cx, global))
	{
		ngx_conf_log_error(NGX_LOG_EMERG, cf, ngx_errno, "Nginx object initialization failed");
		return NGX_CONF_ERROR;
	}
	
	// Nginx.Request
	if (!ngx_http_js__nginx_request__init(cx, global))
	{
		ngx_conf_log_error(NGX_LOG_EMERG, cf, ngx_errno, "Nginx.Request class initialization failed");
		return NGX_CONF_ERROR;
	}
	
	// Nginx.Headers
	if (!ngx_http_js__nginx_headers_in__init(cx, global))
	{
		ngx_conf_log_error(NGX_LOG_EMERG, cf, ngx_errno, "Nginx.HeadersIn class initialization failed");
		return NGX_CONF_ERROR;
	}
	if (!ngx_http_js__nginx_headers_out__init(cx, global))
	{
		ngx_conf_log_error(NGX_LOG_EMERG, cf, ngx_errno, "Nginx.HeadersOut class initialization failed");
		return NGX_CONF_ERROR;
	}
	
	// Nginx.Chain
	if (!ngx_http_js__nginx_chain__init(cx, global))
	{
		ngx_conf_log_error(NGX_LOG_EMERG, cf, ngx_errno, "Nginx.Chain class initialization failed");
		return NGX_CONF_ERROR;
	}
	
	// Nginx.File
	if (!ngx_http_js__nginx_file__init(cx, global))
	{
		ngx_conf_log_error(NGX_LOG_EMERG, cf, ngx_errno, "Nginx.File class initialization failed");
		return NGX_CONF_ERROR;
	}
	
	
	// call some external func
	
	
	if (ngx_http_js_run_loads(cx, global, &jsmcf->loads, cf->log) != NGX_OK)
		return NGX_CONF_ERROR;
	
	if (ngx_http_js_run_requires(cx, global, &jsmcf->requires, cf->log) != NGX_OK)
		return NGX_CONF_ERROR;
	
	ngx_http_js_module_js_runtime = rt;
	ngx_http_js_module_js_context = cx;
	ngx_http_js_module_js_global  = global;
	
	return NGX_CONF_OK;
}


char *
ngx_http_js__glue__set_callback(ngx_conf_t *cf, ngx_command_t *cmd, ngx_http_js_loc_conf_t *jslcf)
{
	ngx_str_t                  *value;
	jsval                       function;
	static char                *JS_CALLBACK_ROOT_NAME = "js callback instance";
	
	{
		ngx_http_js_main_conf_t    *jsmcf;
		jsmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_js_module);
		if (ngx_http_js__glue__init_interpreter(cf, jsmcf) != NGX_CONF_OK)
			return NGX_CONF_ERROR;
	}
	
	value = cf->args->elts;
	if (!JS_EvaluateScript(js_cx, js_global, (char*)value[1].data, value[1].len, (char*)cf->conf_file->file.name.data, cf->conf_file->line, &function))
		return NGX_CONF_ERROR;
	
	if (!JSVAL_IS_OBJECT(function) || !JS_ValueToFunction(js_cx, function))
	{
		ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "result of (%s) is not a function", (char*)value[1].data);
		return NGX_CONF_ERROR;
	}
	
	jslcf->handler_function = JSVAL_TO_OBJECT(function);
	if (!JS_AddNamedRoot(js_cx, &jslcf->handler_function, JS_CALLBACK_ROOT_NAME))
	{
		JS_ReportError(js_cx, "Can`t add new root %s", JS_CALLBACK_ROOT_NAME);
		return NGX_CONF_ERROR;
	}
	
	return NGX_CONF_OK;
}


char *
ngx_http_js__glue__set_filter(ngx_conf_t *cf, ngx_command_t *cmd, ngx_http_js_loc_conf_t *jslcf)
{
	ngx_str_t                  *value;
	jsval                       function;
	static char                *JS_CALLBACK_ROOT_NAME = "js filter instance";
	
	{
		ngx_http_js_main_conf_t    *jsmcf;
		jsmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_js_module);
		if (ngx_http_js__glue__init_interpreter(cf, jsmcf) != NGX_CONF_OK)
			return NGX_CONF_ERROR;
	}
	
	value = cf->args->elts;
	if (!JS_EvaluateScript(js_cx, js_global, (char*)value[1].data, value[1].len, (char*)cf->conf_file->file.name.data, cf->conf_file->line, &function))
		return NGX_CONF_ERROR;
	
	if (!JSVAL_IS_OBJECT(function) || !JS_ValueToFunction(js_cx, function))
	{
		ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "result of (%s) is not a function", (char*)value[1].data);
		return NGX_CONF_ERROR;
	}
	
	jslcf->filter_function = JSVAL_TO_OBJECT(function);
	if (!JS_AddNamedRoot(js_cx, &jslcf->filter_function, JS_CALLBACK_ROOT_NAME))
	{
		JS_ReportError(js_cx, "Can`t add new root %s", JS_CALLBACK_ROOT_NAME);
		return NGX_CONF_ERROR;
	}
	
	
	return NGX_CONF_OK;
}


ngx_int_t
ngx_http_js__glue__call_handler(ngx_http_request_t *r)
{
	ngx_int_t                    rc;
	ngx_log_t                   *last_log;
	JSObject                    *request, *function;
	jsval                        req;
	jsval                        rval;
	ngx_http_js_loc_conf_t      *jslcf;
	ngx_http_js_ctx_t           *ctx;
	
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "js handler");
	
	ngx_assert(js_cx);
	ngx_assert(js_global);
	
	// location configuration for current request
	jslcf = ngx_http_get_module_loc_conf(r, ngx_http_js_module);
	// get the callback function, was set in config by ngx_http_js__glue__set_callback()
	function = jslcf->handler_function;
	ngx_assert(function);
	
	// get a js module context or create a js module context or return an error
	if (!(ctx = ngx_http_get_module_ctx(r, ngx_http_js_module)))
	{
		// ngx_pcalloc fills allocated memory with zeroes
		if ((ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_js_ctx_t))))
			ngx_http_set_ctx(r, ctx, ngx_http_js_module) // ; is in the macro
		else
			return NGX_ERROR;
	}
	
	// create a wrapper js object (not yet rooted) for native request struct
	request = ngx_http_js__nginx_request__wrap(js_cx, r);
	if (request == NULL)
		return NGX_ERROR;
	
	req = OBJECT_TO_JSVAL(request);
	last_log = ngx_http_js_module_log;
	ngx_http_js_module_log = r->connection->log;
	if (JS_CallFunctionValue(js_cx, js_global, OBJECT_TO_JSVAL(function), 1, &req, &rval))
	{
		if (!JSVAL_IS_INT(rval))
		{
			rc = NGX_ERROR;
			JS_ReportError(js_cx, "Request processor must return an Integer");
		}
		else
			rc = (ngx_int_t)JSVAL_TO_INT(rval);
	}
	else
		rc = NGX_HTTP_INTERNAL_SERVER_ERROR;
	ngx_http_js_module_log = last_log;
	
	ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "js handler done: %i (main->count = %i)", rc, r->main->count);
	
	// if timer was set, or subrequest performed, or body is awaited
	if (r->main->count > 2)
	{
		ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "complex request handled, perform GC-related stuff");
		if (ngx_http_js__nginx_request__root_in(ctx, r, js_cx, request) != NGX_OK)
			return NGX_ERROR;
	}
	
	// JS_MaybeGC(js_cx);
	
	if (rc == NGX_DONE)
		return rc;
	
	if (rc > 600)
		rc = NGX_OK;
	
	// if (rc == NGX_OK || rc == NGX_HTTP_OK)
	// 	ngx_http_send_special(r, NGX_HTTP_LAST);
	
	return rc;
}

/*
ngx_int_t
ngx_http_js__glue__call_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
	ngx_int_t                    rc;
	JSObject                    *global, *request, *chain, *function;
	// jsval                        req;
	jsval                        rval, args[2];
	ngx_http_js_loc_conf_t      *jslcf;
	ngx_http_js_main_conf_t     *jsmcf;
	ngx_http_js_ctx_t           *ctx;
	JSContext                   *cx;
	
	TRACE();
	
	ngx_assert(r);
	
	
	jsmcf = ngx_http_get_module_main_conf(r, ngx_http_js_module);
	ngx_assert(jsmcf);
	
	cx = jsmcf->js_cx;
	global = jsmcf->js_global;
	ngx_assert(cx && global);
	
	jslcf = ngx_http_get_module_loc_conf(r, ngx_http_js_module);
	function = jslcf->filter_function;
	ngx_assert(function);
	
	rc = NGX_HTTP_OK;
	
	request = ngx_http_js__nginx_request__wrap(cx, r);
	if (request == NULL)
		return NGX_ERROR;
	
	// ctx was allocated in ngx_http_js__nginx_request__wrap
	ctx = ngx_http_get_module_ctx(r, ngx_http_js_module);
	ngx_assert(ctx);
	
	
	args[0] = OBJECT_TO_JSVAL(request);
	// if (r->upstream)
	// 	args[1] = STRING_TO_JSVAL(JS_NewStringCopyN(cx, (char*) r->upstream->buffer.pos, r->upstream->buffer.last-r->upstream->buffer.pos));
	// else
	if (in)
	{
		chain = ngx_http_js__nginx_chain__wrap(cx, in, request);
		if (!chain)
			return NGX_ERROR;
		args[1] = OBJECT_TO_JSVAL(chain);
	}
	else
		args[1] = JSVAL_VOID;
	
	
	
	// req = OBJECT_TO_JSVAL(request);
	if (JS_CallFunctionValue(cx, global, OBJECT_TO_JSVAL(function), 2, args, &rval))
	{
		if (!JSVAL_IS_INT(rval))
		{
			rc = NGX_ERROR;
			JS_ReportError(cx, "Filter processor must return an Integer");
		}
		else
			rc = (ngx_int_t)JSVAL_TO_INT(rval);
	}
	else
		rc = NGX_ERROR;
	
	
	return rc;
}
*/

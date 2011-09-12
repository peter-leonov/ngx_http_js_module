#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#include <ngx_http_jsapi.h>

#include <ngx_http_js_module.h>
#include <nginx_js_glue.h>
#include <classes/global.h>
#include <classes/environment.h>
#include <classes/Nginx.h>
#include <classes/Request.h>
#include <classes/Request/HeadersIn.h>
#include <classes/Request/HeadersIn/Cookies.h>
#include <classes/Request/HeadersOut.h>
#include <classes/Request/Variables.h>
#include <classes/Chain.h>
#include <classes/File.h>
#include <classes/Dir.h>

#include <strings_util.h>

#include <nginx_js_macroses.h>

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
	
	TRACE();
	
	if (!JS_GetProperty(cx, global, "load", &fval))
	{
		JS_ReportError(cx, "global.load is undefined");
		return JS_FALSE;
	}
	if (!JSVAL_IS_OBJECT(fval) || !JS_ObjectIsFunction(cx, JSVAL_TO_OBJECT(fval)))
	{
		JS_ReportError(cx, "global.load is not a function");
		return JS_FALSE;
	}
	
	fnstring = JS_NewStringCopyZ(cx, filename);
	if (!fnstring)
		return JS_FALSE;
		
	strval = STRING_TO_JSVAL(fnstring);
	
	return JS_CallFunctionValue(cx, global, fval, 1, &strval, &rval);
}

static ngx_int_t
ngx_http_js_run_loads(JSContext *cx, JSObject *global, ngx_array_t *arr, ngx_log_t *log)
{
	char          **elts;
	ngx_uint_t      i;
	char           *value;
	
	TRACE();
	
	elts = arr->elts;
	for (i = 0; i < arr->nelts; i++)
	{
		value = elts[i];
		ngx_log_debug1(NGX_LOG_DEBUG_HTTP, log, 0, "js_load: %s\n", value);
		if (!ngx_http_js_load(cx, global, (char *) value))
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
	if (!JSVAL_IS_OBJECT(fval) || !JS_ObjectIsFunction(cx, JSVAL_TO_OBJECT(fval)))
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
			ngx_log_error(NGX_LOG_EMERG, log, 0, "error calling global.require from nginx");
			return NGX_ERROR;
		}
	}
	
	return NGX_OK;
}



char *
ngx_http_js__glue__init_interpreter(ngx_conf_t *cf)
{
	ngx_http_js_main_conf_t    *jsmcf;
	JSContext                  *cx;
	JSRuntime                  *rt;
	JSObject                   *global;
	
	ngx_log_debug0(NGX_LOG_DEBUG, cf->log, 0, "init interpreter");
	
	if (ngx_http_js_module_js_runtime != NULL)
	{
		// interpreter is already initiated
		return NGX_CONF_OK;
	}
	
	if (ngx_set_environment(cf->cycle, NULL) == NULL)
		return NGX_CONF_ERROR;
	
	
	jsmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_js_module);
	
	rt = JS_NewRuntime(jsmcf->maxmem == NGX_CONF_UNSET_SIZE ? 2L * 1024L * 1024L : jsmcf->maxmem);
	if (rt == NULL)
		return NGX_CONF_ERROR;
	
	cx = JS_NewContext(rt, 8192);
	if (cx == NULL)
		return NGX_CONF_ERROR;
	
	JS_SetOptions(cx, JSOPTION_VAROBJFIX | JSOPTION_JIT | JSOPTION_METHODJIT | JSOPTION_COMPILE_N_GO);
	JS_SetVersion(cx, JSVERSION_LATEST);
	JS_SetErrorReporter(cx, reportError);
	
#if (JS_GC_ZEAL)
	JS_SetGCZeal(cx, 2);
#endif
	
	// global
	if (!ngx_http_js__global__init(cx))
	{
		ngx_conf_log_error(NGX_LOG_EMERG, cf, ngx_errno, "global object initialization failed");
		return NGX_CONF_ERROR;
	}
	
	global = JS_GetGlobalObject(cx);
	
	// environment
	if (!ngx_http_js__environment__init(cx, global))
	{
		ngx_conf_log_error(NGX_LOG_EMERG, cf, ngx_errno, "environment object initialization failed");
		return NGX_CONF_ERROR;
	}
	
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
	
	// Nginx.Cookies
	if (!ngx_http_js__nginx_cookies__init(cx, global))
	{
		ngx_conf_log_error(NGX_LOG_EMERG, cf, ngx_errno, "Nginx.Cookies class initialization failed");
		return NGX_CONF_ERROR;
	}
	
	// Nginx.Variables
	if (!ngx_http_js__nginx_variables__init(cx, global))
	{
		ngx_conf_log_error(NGX_LOG_EMERG, cf, ngx_errno, "Nginx.Request.Variables class initialization failed");
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
	
	// Nginx.Dir
	if (!ngx_http_js__nginx_dir__init(cx, global))
	{
		ngx_conf_log_error(NGX_LOG_EMERG, cf, ngx_errno, "Nginx.Dir class initialization failed");
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

static ngx_int_t
ngx_http_js__glue__destroy_interpreter()
{
	JS_DestroyContext(js_cx);
	JS_DestroyRuntime(ngx_http_js_module_js_runtime);
	JS_ShutDown();
	
	js_cx = NULL;
	js_global = NULL;
	ngx_http_js_module_js_runtime = NULL;
	
	return NGX_OK;
}

ngx_int_t
ngx_http_js__glue__init_worker(ngx_cycle_t *cycle)
{
	jsval           fval, rval;
	JSObject       *global;
	JSContext      *cx;
	
	if (js_cx == NULL || js_global == NULL)
	{
		ngx_log_debug0(NGX_LOG_DEBUG_HTTP, cycle->log, 0, "initiating worker without interpreter has been inited");
	}
	else
	{
		global = js_global;
		cx = js_cx;
		
		if (JS_GetProperty(cx, global, "initWorker", &fval) && JSVAL_IS_OBJECT(fval) && JS_ObjectIsFunction(cx, JSVAL_TO_OBJECT(fval)))
		{
			ngx_log_debug0(NGX_LOG_DEBUG_HTTP, cycle->log, 0, "global.initWorker() found");
			if (!JS_CallFunctionValue(cx, global, fval, 0, NULL, &rval))
			{
				ngx_log_error(NGX_LOG_EMERG, cycle->log, 0, "error calling global.initWorker() from nginx");
				return NGX_ERROR;
			}
		}
	}
	
	return NGX_OK;
}

void
ngx_http_js__glue__exit_worker(ngx_cycle_t *cycle)
{
	jsval           fval, rval;
	JSObject       *global;
	JSContext      *cx;
	
	if (js_cx == NULL || js_global == NULL || ngx_http_js_module_js_runtime == NULL)
	{
		ngx_log_debug0(NGX_LOG_DEBUG_HTTP, cycle->log, 0, "exiting worker without interpreter has been inited");
		return;
	}
	
	global = js_global;
	cx = js_cx;
	
	if (JS_GetProperty(cx, global, "exitWorker", &fval) && JSVAL_IS_OBJECT(fval) && JS_ObjectIsFunction(cx, JSVAL_TO_OBJECT(fval)))
	{
		ngx_log_debug0(NGX_LOG_DEBUG_HTTP, cycle->log, 0, "global.exitWorker() found");
		if (!JS_CallFunctionValue(cx, global, fval, 0, NULL, &rval))
		{
			ngx_log_error(NGX_LOG_EMERG, cycle->log, 0, "error calling global.exitWorker() from nginx");
		}
	}
	
	ngx_http_js__glue__destroy_interpreter();
}

void
ngx_http_js__glue__exit_master(ngx_cycle_t *cycle)
{
	jsval           fval, rval;
	JSObject       *global;
	JSContext      *cx;
	
	if (js_cx == NULL || js_global == NULL || ngx_http_js_module_js_runtime == NULL)
	{
		ngx_log_debug0(NGX_LOG_DEBUG_HTTP, cycle->log, 0, "exiting master without interpreter has been inited");
		return;
	}
	
	global = js_global;
	cx = js_cx;
	
	if (JS_GetProperty(cx, global, "exitMaster", &fval) && JSVAL_IS_OBJECT(fval) && JS_ObjectIsFunction(cx, JSVAL_TO_OBJECT(fval)))
	{
		ngx_log_debug0(NGX_LOG_DEBUG_HTTP, cycle->log, 0, "global.exitMaster() found");
		if (!JS_CallFunctionValue(cx, global, fval, 0, NULL, &rval))
		{
			ngx_log_error(NGX_LOG_EMERG, cycle->log, 0, "error calling global.exitMaster() from nginx");
		}
	}
	
	ngx_http_js__glue__destroy_interpreter();
}


char *
ngx_http_js__glue__set_handler(ngx_conf_t *cf, JSObject **handler, const char *root_name)
{
	ngx_str_t                  *value;
	jsval                       function;
	
	if (ngx_http_js__glue__init_interpreter(cf) != NGX_CONF_OK)
	{
		return NGX_CONF_ERROR;
	}
	
	value = cf->args->elts;
	if (!JS_EvaluateScript(js_cx, js_global, (char*)value[1].data, value[1].len, (char*)cf->conf_file->file.name.data, cf->conf_file->line, &function))
		return NGX_CONF_ERROR;
	
	if (!JSVAL_IS_OBJECT(function) || !JS_ObjectIsFunction(js_cx, JSVAL_TO_OBJECT(function)))
	{
		ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "result of (%s) is not a function", (char*)value[1].data);
		return NGX_CONF_ERROR;
	}
	
	*handler = JSVAL_TO_OBJECT(function);
	if (!JS_AddNamedObjectRoot(js_cx, handler, root_name))
	{
		JS_ReportError(js_cx, "Can`t add new root %s", root_name);
		return NGX_CONF_ERROR;
	}
	
	return NGX_CONF_OK;
}

static ngx_int_t
js_variable_get_handler(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data)
{
	ngx_http_js_variable_t      *jv;
	ngx_str_t                    value;
	jsval                        rval;
	JSString                    *js_value;
	JSContext                   *cx;
	
	TRACE();
	
	jv = (ngx_http_js_variable_t *) data;
	cx = js_cx;
	
	if (!ngx_http_js__request__call_function(cx, r, jv->function, &rval))
	{
		return NGX_ERROR;
	}
	
	// if the callback returns undefined we mark the variable as not_found
	if (JSVAL_IS_VOID(rval))
	{
		v->not_found = 1;
		return NGX_OK;
	}
	
	// otherwise, convert whatever returned to the string
	js_value = JS_ValueToString(cx, rval);
	if (js_value == NULL)
	{
		return NGX_ERROR;
	}
	
	// allocate the C-string data on the request pool
	if (!js_str2ngx_str(cx, js_value, r->pool, &value))
	{
		// likely OOM
		return NGX_ERROR;
	}
	
	v->data = value.data;
	v->len = value.len;
	v->valid = 1;
	v->no_cacheable = 0;
	v->not_found = 0;
	
	return NGX_OK;
}

char *
ngx_http_js__glue__js_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	ngx_int_t                   index;
	ngx_str_t                  *value;
	ngx_http_variable_t        *v;
	ngx_http_js_variable_t     *jv;
	
	TRACE();
	
	value = cf->args->elts;
	
	if (value[1].data[0] != '$')
	{
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid variable name \"%V\"", &value[1]);
		return NGX_CONF_ERROR;
	}
	
	value[1].len--;
	value[1].data++;
	
	v = ngx_http_add_variable(cf, &value[1], NGX_HTTP_VAR_CHANGEABLE);
	if (v == NULL)
	{
		return NGX_CONF_ERROR;
	}
	
	jv = ngx_palloc(cf->pool, sizeof(ngx_http_js_variable_t));
	if (jv == NULL)
	{
		return NGX_CONF_ERROR;
	}
	
	index = ngx_http_get_variable_index(cf, &value[1]);
	if (index == NGX_ERROR)
	{
		return NGX_CONF_ERROR;
	}
	
	jv->handler = value[2];
	
	{
		jsval                       function;
		static char                *JS_VARIABLE_CALLBACK_ROOT_NAME = "js_set callback instance";
		
		if (ngx_http_js__glue__init_interpreter(cf) != NGX_CONF_OK)
		{
			return NGX_CONF_ERROR;
		}
		
		if (!JS_EvaluateScript(js_cx, js_global, (char *) value[2].data, value[2].len, (char *) cf->conf_file->file.name.data, cf->conf_file->line, &function))
		{
			return NGX_CONF_ERROR;
		}
		
		if (!JSVAL_IS_OBJECT(function) || !JS_ObjectIsFunction(js_cx, JSVAL_TO_OBJECT(function)))
		{
			ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "result of \"%*s\" is not a function", value[2].len, (char *) value[2].data);
			return NGX_CONF_ERROR;
		}
		
		jv->function = JSVAL_TO_OBJECT(function);
		if (!JS_AddNamedObjectRoot(js_cx, &jv->function, JS_VARIABLE_CALLBACK_ROOT_NAME))
		{
			JS_ReportError(js_cx, "Can`t add new root %s", JS_VARIABLE_CALLBACK_ROOT_NAME);
			return NGX_CONF_ERROR;
		}
	}
	
	v->get_handler = js_variable_get_handler;
	v->data = (uintptr_t) jv;
	
	return NGX_CONF_OK;
}

ngx_int_t
ngx_http_js__glue__access_handler(ngx_http_request_t *r)
{
	ngx_int_t                    rc;
	jsval                        rval;
	ngx_http_js_loc_conf_t      *jslcf;	
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "js access handler");
	
	// location configuration for current request
	jslcf = ngx_http_get_module_loc_conf(r, ngx_http_js_module);
	if (jslcf == NULL || jslcf->access_handler_function == NULL)
	{
		return NGX_DECLINED;
	}
	
	// the callback function was set in config by ngx_http_js__glue__set_handler()
	if (!ngx_http_js__request__call_function(js_cx, r, jslcf->access_handler_function, &rval))
	{
		return NGX_ERROR;
	}
	
	rc = JSVAL_IS_INT(rval) ? JSVAL_TO_INT(rval) : NGX_OK;
	
	// JS_MaybeGC(js_cx);
	
	return rc;
}

ngx_int_t
ngx_http_js__glue__content_handler(ngx_http_request_t *r)
{
	ngx_int_t                    rc;
	jsval                        rval;
	ngx_http_js_loc_conf_t      *jslcf;	
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "js content handler");
	
	// location configuration for current request
	jslcf = ngx_http_get_module_loc_conf(r, ngx_http_js_module);
	
	
	// the callback function was set in config by ngx_http_js__glue__set_handler()
	if (!ngx_http_js__request__call_function(js_cx, r, jslcf->content_handler_function, &rval))
	{
		return NGX_ERROR;
	}
	
	rc = JSVAL_IS_INT(rval) ? JSVAL_TO_INT(rval) : NGX_OK;
	
	// JS_MaybeGC(js_cx);
	
	if (rc == NGX_DONE)
		return rc;
	
	if (rc > 600)
		rc = NGX_OK;
	
	// if (rc == NGX_OK || rc == NGX_HTTP_OK)
	// 	ngx_http_send_special(r, NGX_HTTP_LAST);
	
	return rc;
}

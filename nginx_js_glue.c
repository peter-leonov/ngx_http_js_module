#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#include <js/jsapi.h>


static ngx_str_t  ngx_null_name = ngx_null_string;


void
reportError(JSContext *cx, const char *message, JSErrorReport *report)
{
	ngx_http_js_context_private_t  *private;
	
	private = JS_GetContextPrivate(cx);
	
	if (!private)
		return;
	
	ngx_log_error
	(
		NGX_LOG_ERR, private->log, 0,
		"%s:%i: %s",
		report->filename ? report->filename : "<no filename>",
		(unsigned int) report->lineno,
		message
	);
}




static ngx_int_t
ngx_http_js_load(JSContext *cx, JSObject *global, char *filename)
{
	jsval           fval, rval, strval;
	JSString       *fnstring;
	// JSObject       *require;
	
	if (!JS_GetProperty(cx, global, "load", &fval))
	{
		JS_ReportError(cx, "global.load is undefined");
		return NGX_ERROR;
	}
	if (!JSVAL_IS_OBJECT(fval) || !JS_ValueToFunction(cx, fval))
	{
		JS_ReportError(cx, "global.load is not a function");
		return NGX_ERROR;
	}
	
	fnstring = JS_NewStringCopyZ(cx, filename);
	if (!fnstring)
		return NGX_ERROR;
		
	strval = STRING_TO_JSVAL(fnstring);
	
	if (!JS_CallFunctionValue(cx, global, fval, 1, &strval, &rval))
	{
		JS_ReportError(cx, "error calling global.load from nginx");
		return NGX_ERROR;
	}
	
	
	// filename = JS_NewStringCopyZ(cx, NGX_HTTP_JS_CONF_PATH);
	// strval = STRING_TO_JSVAL(filename);
	// if (!JS_SetProperty(cx, global, "__FILE__", &strval))
	// {
	// 	JS_ReportError(cx, "unable to set global.__FILE__ to '%s'", NGX_HTTP_JS_CONF_PATH);
	// 	return NGX_ERROR;
	// }
	// 
	// jss = JS_CompileFile(cx, global, NGX_HTTP_JS_CONF_PATH);
	// if (!jss)
	// {
	// 	JS_ReportError(cx, "error compiling NGX_HTTP_JS_CONF_PATH");
	// 	return NGX_ERROR;
	// }
	// 
	// if (!JS_ExecuteScript(cx, global, jss, &rval))
	// {
	// 	JS_ReportError(cx, "error executing NGX_HTTP_JS_CONF_PATH");
	// 	return NGX_ERROR;
	// }
	
	return NGX_OK;
}

static ngx_int_t
ngx_http_js_run_requires(JSContext *cx, JSObject *global, ngx_array_t *requires, ngx_log_t *log)
{
	// ngx_str_t     **script;
	char          **script;
	ngx_uint_t      i;
	jsval           rval, strval, fval;
	// ngx_str_t      *value;
	char           *value;
	// JSObject       *require;
	
	if (ngx_http_js_load(cx, global, NGX_HTTP_JS_CONF_PATH) != NGX_OK)
		return NGX_ERROR;
	
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
		// fprintf(stderr, "load %s\n", value);
		
		strval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, (char*)value));
		if (!JS_CallFunctionValue(cx, global, fval, 1, &strval, &rval))
		{
			JS_ReportError(cx, "error calling global.require from nginx");
			return NGX_ERROR;
		}
	}
	
	return NGX_OK;
}







static char *
ngx_http_js__nginx_js__init_interpreter(ngx_conf_t *cf, ngx_http_js_main_conf_t *jsmcf)
{
	static JSRuntime *rt;
	static JSContext *static_cx = NULL;
	static JSObject  *global;
	JSContext        *cx;
	
	if (jsmcf->js_cx != NULL)
		return NGX_CONF_OK;
	
	if (static_cx)
	{
		if (ngx_set_environment(cf->cycle, NULL) == NULL)
			return NGX_CONF_ERROR;
		
		jsmcf->js_cx = static_cx;
		jsmcf->js_global = global;
		
		return NGX_CONF_OK;
	}
	
	
	rt = JS_NewRuntime(32L * 1024L * 1024L);
	if (rt == NULL)
		return NGX_CONF_ERROR;
	
	cx = JS_NewContext(rt, 8192);
	if (cx == NULL)
		return NGX_CONF_ERROR;
	
	JS_SetOptions(cx, JSOPTION_VAROBJFIX);
	JS_SetVersion(cx, 170);
	JS_SetErrorReporter(cx, reportError);
	
	global = JS_NewObject(cx, &global_class, NULL, NULL);
	if (global == NULL)
		return NGX_CONF_ERROR;
	
	JS_SetGlobalObject(cx, global);
	JS_DefineProperty(cx, global, "self", OBJECT_TO_JSVAL(global), NULL, NULL, 0);
	
	if (!JS_InitStandardClasses(cx, global))
		return NGX_CONF_ERROR;
	
	
	
	
	// if (ngx_http_js_init_interpreter_nginx(cf, jsmcf, cx, global) != NGX_OK)
	// 	return NGX_CONF_ERROR;
	
	static ngx_http_js_context_private_t   private;
	JSObject *nginxobj;
	
	if (JS_GetContextPrivate(cx))
		return NGX_ERROR;
	
	private.cf = cf;
	private.jsmcf = jsmcf;
	private.log = cf->log;
	
	JS_SetContextPrivate(cx, &private);
	

	// call some external func
	
	
	return NGX_OK;
	
	
	
	
	jsmcf->js_cx = static_cx = cx;
	jsmcf->js_global = global;
	
	if (ngx_http_js_run_requires(static_cx, global, &jsmcf->requires, cf->log) != NGX_OK)
		return NGX_CONF_ERROR;
	
	return NGX_CONF_OK;
}





static char *
ngx_http_js__nginx_js_set_callback(ngx_conf_t *cf, ngx_command_t *cmd, ngx_http_js_loc_conf_t *jslcf)
{
	ngx_str_t                  *value;
	ngx_http_core_loc_conf_t   *clcf;
	ngx_http_js_main_conf_t    *jsmcf;
	jsval                       sub;
	
	value = cf->args->elts;
	// fprintf(stderr, "js %s\n", value[1].data);
	if (jslcf->handler.data)
	{
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "duplicate js handler \"%V\"", &value[1]);
		return NGX_CONF_ERROR;
	}
	
	jsmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_js_module);
	
	if (jsmcf->js_cx == NULL)
		if (ngx_http_js_init_interpreter(cf, jsmcf) != NGX_CONF_OK)
			return NGX_CONF_ERROR;
	
	jslcf->handler = value[1];
	
	if (!JS_EvaluateScript(jsmcf->js_cx, jsmcf->js_global, (char*)value[1].data, value[1].len, (char*)cf->conf_file->file.name.data, cf->conf_file->line, &sub))
		return NGX_CONF_ERROR;
	
	if (!JSVAL_IS_OBJECT(sub) || !JS_ValueToFunction(jsmcf->js_cx, sub))
	{
		ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "processRequest is not a function");
		return NGX_CONF_ERROR;
	}
	
	jslcf->sub = JSVAL_TO_OBJECT(sub);
	
	// JS side of question
	if (ngx_http_js__nginx_js_set_callback(cf, cmd, jsmcf) != NGX_CONF_OK)
		return NGX_CONF_ERROR;
	
	
	clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
	clcf->handler = ngx_http_js_handler;
	
	return NGX_CONF_OK;
}




//#define unless(a) if(!(a))

static ngx_int_t
ngx_http_js__nginx_js_call_handler(JSContext *cx, JSObject *global, ngx_http_request_t *r, JSObject *sub, ngx_str_t *handler)
{
	int                status;
	ngx_connection_t  *c;
	JSObject          *request;
	jsval              req;
	// fprintf(stderr, "%s", (char *) r->uri.data);
	status = NGX_HTTP_OK;
	jsval              rval;
	static char        *JS_REQUEST_ROOT_NAME = "Nginx.Request instance";
	
	// ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "ngx_http_js_call_handler(%p)", r);
	
	request = JS_NewObject(cx, &js_nginx_request_class, ngx_http_js__request_prototype, NULL);
	if (!request)
	{
		JS_ReportOutOfMemory(cx);
		return NGX_ERROR;
	}
	
	if (!JS_AddNamedRoot(cx, &request, JS_REQUEST_ROOT_NAME))
	{
		JS_ReportError(cx, "Can`t add new root %s", JS_REQUEST_ROOT_NAME);
		return NGX_ERROR;
	}
	JS_SetPrivate(cx, request, r);
	req = OBJECT_TO_JSVAL(request);
	
	c = r->connection;
	
	if (JS_CallFunctionValue(cx, global, OBJECT_TO_JSVAL(sub), 1, &req, &rval))
	{
		if (!JSVAL_IS_INT(rval))
		{
			status = NGX_ERROR;
			JS_ReportError(cx, "Request processor must return an Integer");
		}
		else
			status = JSVAL_TO_INT(rval);
	}
	else
		status = NGX_ERROR;
	
	// if (r->headers_out.status == 0)
	// 	r->headers_out.status = NGX_HTTP_OK;
	// ngx_http_send_header(r);
	
	JS_SetPrivate(cx, request, NULL);
	
	JS_MaybeGC(cx);
	
	if (c->destroyed)
		return NGX_DONE;
	// fprintf(stderr, "%d", status);
	return (ngx_int_t) status;
}

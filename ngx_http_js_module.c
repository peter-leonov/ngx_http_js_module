
/*
 * Copyright (C) Peter Leonov
 */

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_http_js_module.h>

static void *ngx_http_js_create_main_conf(ngx_conf_t *cf);
static char *ngx_http_js_init_main_conf(ngx_conf_t *cf, void *conf);
static char *ngx_http_js_require(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_js(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_js_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static ngx_int_t ngx_http_js_call_handler(JSContext *cx, JSObject *global, ngx_http_request_t *r, JSObject *sub, ngx_str_t *handler);
static char *ngx_http_js_init_interpreter(ngx_conf_t *cf, ngx_http_js_main_conf_t *jsmcf);
static void *ngx_http_js_create_loc_conf(ngx_conf_t *cf);
static ngx_int_t ngx_http_js_init_interpreter_nginx(ngx_conf_t *cf, ngx_http_js_main_conf_t *jsmcf, JSContext *cx, JSObject  *global);


static ngx_str_t  ngx_null_name = ngx_null_string;

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


static JSClass global_class =
{
    "global", JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

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

void
ngx_http_js_handle_request(ngx_http_request_t *r)
{
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
			return;
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
	
	rc = ngx_http_js_call_handler(jsmcf->js, jsmcf->global, r, sub, handler);

    }

    if (rc == NGX_DONE) {
        return;
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
        return;
    }

    if (uri.len) {
        ngx_http_internal_redirect(r, &uri, &args);
        return;
    }

    if (rc == NGX_OK || rc == NGX_HTTP_OK) {
        ngx_http_send_special(r, NGX_HTTP_LAST);
        ctx->done = 1;
    }

    ngx_http_finalize_request(r, rc);
}

static ngx_int_t
ngx_http_js_handler(ngx_http_request_t *r)
{
	if (r->zero_in_uri)
		return NGX_HTTP_NOT_FOUND;
	
	ngx_http_js_handle_request(r);
	
	return NGX_DONE;
}

// static ngx_int_t
// ngx_http_js_run_requires(JSContext *cx, JSObject *global, ngx_array_t *requires, ngx_log_t *log)
// {
//  char       **script;
//  ngx_uint_t   i;
//  JSScript    *jss;
//  jsval       rval;
//  
//  script = requires->elts;
//  for (i = 0; i < requires->nelts; i++) {
//      fprintf(stderr, "load %s\n", script[i]);
//      
//      jss = JS_CompileFile(cx, global, script[i]);
//      if (!jss)
//          return NGX_ERROR;
//      
//      if (!JS_ExecuteScript(cx, global, jss, &rval))
//          return NGX_ERROR;
//  }
//  
//  return NGX_OK;
// }

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
	ngx_http_js_main_conf_t    *jsmcf;
	jsval                       sub;
	
	value = cf->args->elts;
	fprintf(stderr, "js %s\n", value[1].data);
	if (jslcf->handler.data)
	{
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "duplicate js handler \"%V\"", &value[1]);
		return NGX_CONF_ERROR;
	}
	
	jsmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_js_module);
	
	if (jsmcf->js == NULL)
		if (ngx_http_js_init_interpreter(cf, jsmcf) != NGX_CONF_OK)
			return NGX_CONF_ERROR;
	
	jslcf->handler = value[1];
	
	// ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "js = %p, global = %p", jsmcf->js, jsmcf->global);
	// if (!JS_GetProperty(jsmcf->js, jsmcf->global, "processRequest", &sub))
		// return NGX_CONF_ERROR;
	
	if (!JS_EvaluateScript(jsmcf->js, jsmcf->global, (char*)value[1].data, value[1].len, (char*)cf->conf_file->file.name.data, cf->conf_file->line, &sub))
		return NGX_CONF_ERROR;
	
	if (!JSVAL_IS_OBJECT(sub) || !JS_ValueToFunction(jsmcf->js, sub))
	{
		ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "processRequest is not a function");
		return NGX_CONF_ERROR;
	}
	
	jslcf->sub = JSVAL_TO_OBJECT(sub);
	
	clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
	clcf->handler = ngx_http_js_handler;
	
	return NGX_CONF_OK;
}

static char *
ngx_http_js_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	return NGX_CONF_OK;
}


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
    ngx_http_js_main_conf_t *jsmcf = conf;

    if (jsmcf->js == NULL) {
        if (ngx_http_js_init_interpreter(cf, jsmcf) != NGX_CONF_OK) {
            return NGX_CONF_ERROR;
        }
    }

    return NGX_CONF_OK;
}

static char *
ngx_http_js_init_interpreter(ngx_conf_t *cf, ngx_http_js_main_conf_t *jsmcf)
{
	static JSRuntime *rt;
	static JSContext *static_cx = NULL;
	static JSObject  *global;
	JSContext        *cx;
	
	if (static_cx)
	{
		if (ngx_set_environment(cf->cycle, NULL) == NULL)
			return NGX_CONF_ERROR;
		
		jsmcf->js = static_cx;
		jsmcf->global = global;
		
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
	
	if (ngx_http_js_init_interpreter_nginx(cf, jsmcf, cx, global) != NGX_OK)
		return NGX_CONF_ERROR;
	
	jsmcf->js = static_cx = cx;
	jsmcf->global = global;
	
	if (ngx_http_js_run_requires(static_cx, global, &jsmcf->requires, cf->log) != NGX_OK)
		return NGX_CONF_ERROR;
	
	return NGX_CONF_OK;
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



// partly must be separated in JS module

// Nginx

static JSBool
js_global_func_load(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
{
	uintN i;
	JSString *str;
	const char *filename, *filevar;
	JSScript *script;
	JSBool ok;
	jsval result, name, old;
	uint32 oldopts;
	JSObject *global;
	// FILE *fileh;
	
	global = JS_GetGlobalObject(cx);
	filevar = "__FILE__";
	
	for (i = 0; i < argc; i++)
	{
		str = JS_ValueToString(cx, argv[i]);
		if (!str)
			return JS_FALSE;
		name = argv[i] = STRING_TO_JSVAL(str);
		filename = JS_GetStringBytes(str);
		fprintf(stderr, "global.load %s\n", filename);
		errno = 0;
		oldopts = JS_GetOptions(cx);
		JS_SetOptions(cx, oldopts | JSOPTION_COMPILE_N_GO);
		script = JS_CompileFile(cx, this, filename);
		// fprintf(stderr, "Error: %i.\n", strerror(errno));
		// if (errno == ENOENT)
		if (!script)
			ok = JS_FALSE;
		else
		{
			JS_GetProperty(cx, global, filevar, &old);
			JS_SetProperty(cx, global, filevar, &name);
			ok = JS_ExecuteScript(cx, this, script, &result);
			JS_SetProperty(cx, global, filevar, &old);
		}
		JS_SetOptions(cx, oldopts);
		if (!ok)
			return JS_FALSE;
	}
	
	return JS_TRUE;
}



static JSBool
js_nginx_class_func_log_error(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
{
	ngx_http_js_context_private_t  *private;
	JSString                       *jsstr;
	ngx_uint_t                      level;
	
	private = JS_GetContextPrivate(cx);
	if (!private)
		return JS_FALSE;
	
	if (argc != 2 || !JSVAL_IS_INT(argv[0]) || !JSVAL_IS_STRING(argv[1]))
	{
		JS_ReportError(cx, "Nginx.logError takes 2 arguments of types Integer and String");
		return JS_FALSE;
	}
	
	level = JSVAL_TO_INT(argv[0]);
	jsstr = JSVAL_TO_STRING(argv[1]);
	
	ngx_log_error(level, private->log, 0, "%s", JS_GetStringBytes(jsstr));
	// ngx_log_error(level, private->log, 0, "%d", JS_GetStringLength(jsstr));
	
	return JS_TRUE;
}

static JSFunctionSpec js_nginx_class_funcs[] = {
    {"logError",    js_nginx_class_func_log_error,         1, 0, 0},
    {0, NULL, 0, 0, 0}
};

static JSPropertySpec js_nginx_class_props[] =
{
	// NGX_LOG*
	{"LOG_STDERR", 1, JSPROP_READONLY, NULL, NULL},
	{"LOG_EMERG", 2, JSPROP_READONLY, NULL, NULL},
	{"LOG_ALERT", 3, JSPROP_READONLY, NULL, NULL},
	{"LOG_CRIT", 4, JSPROP_READONLY, NULL, NULL},
	{"LOG_ERR", 5, JSPROP_READONLY, NULL, NULL},
	{"LOG_WARN", 6, JSPROP_READONLY, NULL, NULL},
	{"LOG_NOTICE", 7, JSPROP_READONLY, NULL, NULL},
	{"LOG_INFO", 8, JSPROP_READONLY, NULL, NULL},
	{"LOG_DEBUG", 9, JSPROP_READONLY, NULL, NULL},
	
	// NGX_HTTP*
	{"HTTP_OK", 10, JSPROP_READONLY, NULL, NULL},
	{"HTTP_CREATED", 11, JSPROP_READONLY, NULL, NULL},
	{"HTTP_NO_CONTENT", 12, JSPROP_READONLY, NULL, NULL},
	{"HTTP_PARTIAL_CONTENT", 13, JSPROP_READONLY, NULL, NULL},
	{"HTTP_SPECIAL_RESPONSE", 14, JSPROP_READONLY, NULL, NULL},
	{"HTTP_MOVED_PERMANENTLY", 15, JSPROP_READONLY, NULL, NULL},
	{"HTTP_MOVED_TEMPORARILY", 16, JSPROP_READONLY, NULL, NULL},
	{"HTTP_NOT_MODIFIED", 17, JSPROP_READONLY, NULL, NULL},
	{"HTTP_BAD_REQUEST", 18, JSPROP_READONLY, NULL, NULL},
	{"HTTP_UNAUTHORIZED", 19, JSPROP_READONLY, NULL, NULL},
	{"HTTP_FORBIDDEN", 20, JSPROP_READONLY, NULL, NULL},
	{"HTTP_NOT_FOUND", 21, JSPROP_READONLY, NULL, NULL},
	{"HTTP_NOT_ALLOWED", 22, JSPROP_READONLY, NULL, NULL},
	{"HTTP_REQUEST_TIME_OUT", 23, JSPROP_READONLY, NULL, NULL},
	{"HTTP_CONFLICT", 24, JSPROP_READONLY, NULL, NULL},
	{"HTTP_LENGTH_REQUIRED", 25, JSPROP_READONLY, NULL, NULL},
	{"HTTP_PRECONDITION_FAILED", 26, JSPROP_READONLY, NULL, NULL},
	{"HTTP_REQUEST_ENTITY_TOO_LARGE", 27, JSPROP_READONLY, NULL, NULL},
	{"HTTP_REQUEST_URI_TOO_LARGE", 28, JSPROP_READONLY, NULL, NULL},
	{"HTTP_UNSUPPORTED_MEDIA_TYPE", 29, JSPROP_READONLY, NULL, NULL},
	{"HTTP_RANGE_NOT_SATISFIABLE", 30, JSPROP_READONLY, NULL, NULL},
	{"HTTP_CLOSE", 31, JSPROP_READONLY, NULL, NULL},
	{"HTTP_OWN_CODES", 32, JSPROP_READONLY, NULL, NULL},
	{"HTTPS_CERT_ERROR", 33, JSPROP_READONLY, NULL, NULL},
	{"HTTPS_NO_CERT", 34, JSPROP_READONLY, NULL, NULL},
	{"HTTP_TO_HTTPS", 35, JSPROP_READONLY, NULL, NULL},
	{"HTTP_CLIENT_CLOSED_REQUEST", 36, JSPROP_READONLY, NULL, NULL},
	{"HTTP_INTERNAL_SERVER_ERROR", 37, JSPROP_READONLY, NULL, NULL},
	{"HTTP_NOT_IMPLEMENTED", 38, JSPROP_READONLY, NULL, NULL},
	{"HTTP_BAD_GATEWAY", 39, JSPROP_READONLY, NULL, NULL},
	{"HTTP_SERVICE_UNAVAILABLE", 40, JSPROP_READONLY, NULL, NULL},
	{"HTTP_GATEWAY_TIME_OUT", 41, JSPROP_READONLY, NULL, NULL},
	{"HTTP_INSUFFICIENT_STORAGE", 42, JSPROP_READONLY, NULL, NULL},
    {0, 0, 0, NULL, NULL}
};

static JSBool
js_nginx_class_getProperty(JSContext *cx, JSObject *this, jsval id, jsval *vp)
{
	// fprintf(stderr, "Nginx property id = %d\n", JSVAL_TO_INT(id));
	if (JSVAL_IS_INT(id))
	{
		switch (JSVAL_TO_INT(id))
		{
			// NGX_LOG*
			case 1: *vp = INT_TO_JSVAL(NGX_LOG_STDERR); break;
			case 2: *vp = INT_TO_JSVAL(NGX_LOG_EMERG); break;
			case 3: *vp = INT_TO_JSVAL(NGX_LOG_ALERT); break;
			case 4: *vp = INT_TO_JSVAL(NGX_LOG_CRIT); break;
			case 5: *vp = INT_TO_JSVAL(NGX_LOG_ERR); break;
			case 6: *vp = INT_TO_JSVAL(NGX_LOG_WARN); break;
			case 7: *vp = INT_TO_JSVAL(NGX_LOG_NOTICE); break;
			case 8: *vp = INT_TO_JSVAL(NGX_LOG_INFO); break;
			case 9: *vp = INT_TO_JSVAL(NGX_LOG_DEBUG); break;
			
			// NGX_HTTP*
			case 10: *vp = INT_TO_JSVAL(NGX_HTTP_OK); break;
			case 11: *vp = INT_TO_JSVAL(NGX_HTTP_CREATED); break;
			case 12: *vp = INT_TO_JSVAL(NGX_HTTP_NO_CONTENT); break;
			case 13: *vp = INT_TO_JSVAL(NGX_HTTP_PARTIAL_CONTENT); break;
			case 14: *vp = INT_TO_JSVAL(NGX_HTTP_SPECIAL_RESPONSE); break;
			case 15: *vp = INT_TO_JSVAL(NGX_HTTP_MOVED_PERMANENTLY); break;
			case 16: *vp = INT_TO_JSVAL(NGX_HTTP_MOVED_TEMPORARILY); break;
			case 17: *vp = INT_TO_JSVAL(NGX_HTTP_NOT_MODIFIED); break;
			case 18: *vp = INT_TO_JSVAL(NGX_HTTP_BAD_REQUEST); break;
			case 19: *vp = INT_TO_JSVAL(NGX_HTTP_UNAUTHORIZED); break;
			case 20: *vp = INT_TO_JSVAL(NGX_HTTP_FORBIDDEN); break;
			case 21: *vp = INT_TO_JSVAL(NGX_HTTP_NOT_FOUND); break;
			case 22: *vp = INT_TO_JSVAL(NGX_HTTP_NOT_ALLOWED); break;
			case 23: *vp = INT_TO_JSVAL(NGX_HTTP_REQUEST_TIME_OUT); break;
			case 24: *vp = INT_TO_JSVAL(NGX_HTTP_CONFLICT); break;
			case 25: *vp = INT_TO_JSVAL(NGX_HTTP_LENGTH_REQUIRED); break;
			case 26: *vp = INT_TO_JSVAL(NGX_HTTP_PRECONDITION_FAILED); break;
			case 27: *vp = INT_TO_JSVAL(NGX_HTTP_REQUEST_ENTITY_TOO_LARGE); break;
			case 28: *vp = INT_TO_JSVAL(NGX_HTTP_REQUEST_URI_TOO_LARGE); break;
			case 29: *vp = INT_TO_JSVAL(NGX_HTTP_UNSUPPORTED_MEDIA_TYPE); break;
			case 30: *vp = INT_TO_JSVAL(NGX_HTTP_RANGE_NOT_SATISFIABLE); break;
			case 31: *vp = INT_TO_JSVAL(NGX_HTTP_CLOSE); break;
			case 32: *vp = INT_TO_JSVAL(NGX_HTTP_OWN_CODES); break;
			case 33: *vp = INT_TO_JSVAL(NGX_HTTPS_CERT_ERROR); break;
			case 34: *vp = INT_TO_JSVAL(NGX_HTTPS_NO_CERT); break;
			case 35: *vp = INT_TO_JSVAL(NGX_HTTP_TO_HTTPS); break;
			case 36: *vp = INT_TO_JSVAL(NGX_HTTP_CLIENT_CLOSED_REQUEST); break;
			case 37: *vp = INT_TO_JSVAL(NGX_HTTP_INTERNAL_SERVER_ERROR); break;
			case 38: *vp = INT_TO_JSVAL(NGX_HTTP_NOT_IMPLEMENTED); break;
			case 39: *vp = INT_TO_JSVAL(NGX_HTTP_BAD_GATEWAY); break;
			case 40: *vp = INT_TO_JSVAL(NGX_HTTP_SERVICE_UNAVAILABLE); break;
			case 41: *vp = INT_TO_JSVAL(NGX_HTTP_GATEWAY_TIME_OUT); break;
			case 42: *vp = INT_TO_JSVAL(NGX_HTTP_INSUFFICIENT_STORAGE); break;
		}
	}
	return JS_TRUE;
}


static JSClass js_nginx_class =
{
    "Nginx",
    0,
    JS_PropertyStub, JS_PropertyStub, js_nginx_class_getProperty, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};



// Nginx.Request

enum js_nginx_request_class_propid { JS_REQUEST_URI, JS_REQUEST_METHOD, JS_REQUEST_REMOTE_ADDR };

static JSBool
js_nginx_request_class_getProperty(JSContext *cx, JSObject *this, jsval id, jsval *vp)
{
	ngx_http_request_t *r;
	r = (ngx_http_request_t *) JS_GetPrivate(cx, this);
	if (!r)
	{
		JS_ReportError(cx, "Trying to use a Request instance with a NULL native request pointer");
		return JS_FALSE;
	}
	
	// fprintf(stderr, "Nginx.Request property id = %d\n", JSVAL_TO_INT(id));
	if (JSVAL_IS_INT(id))
	{
		switch (JSVAL_TO_INT(id))
		{
			case JS_REQUEST_URI:
			*vp = STRING_TO_JSVAL(JS_NewStringCopyN(cx, (char *) r->uri.data, r->uri.len));
			break;
			
			case JS_REQUEST_METHOD:
			*vp = STRING_TO_JSVAL(JS_NewStringCopyN(cx, (char *) r->method_name.data, r->method_name.len));
			break;
			
			case JS_REQUEST_REMOTE_ADDR:
			*vp = STRING_TO_JSVAL(JS_NewStringCopyN(cx, (char *) r->connection->addr_text.data, r->connection->addr_text.len));
			break;
			
		}
	}
	return JS_TRUE;
}

static JSClass js_nginx_request_class =
{
	"Request",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub, js_nginx_request_class_getProperty, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSPropertySpec js_nginx_request_class_props[] =
{
	{"uri",      JS_REQUEST_URI,          JSPROP_READONLY,   NULL, NULL},
	// {"discardRequestBody",       MY_COLOR,       JSPROP_ENUMERATE,  NULL, NULL},
	{"method",   JS_REQUEST_METHOD,       JSPROP_READONLY,   NULL, NULL},
	// {"headerOnly",       MY_COLOR,       JSPROP_ENUMERATE,  NULL, NULL},
	{"remoteAddr",      JS_REQUEST_REMOTE_ADDR,      JSPROP_READONLY,  NULL, NULL},
	// {"status",       MY_WIDTH,       JSPROP_ENUMERATE,  NULL, NULL},
	// {"requestBody",       MY_FUNNY,       JSPROP_ENUMERATE,  NULL, NULL},
	// {"requestBodyFile",       MY_ARRAY,       JSPROP_ENUMERATE,  NULL, NULL},
	// {"allowRanges",       MY_ARRAY,       JSPROP_ENUMERATE,  NULL, NULL},
	// {"headerOnly",      MY_RDONLY,      JSPROP_READONLY,   NULL, NULL},
	{0, 0, 0, NULL, NULL}
};


static ngx_buf_t *
js_str2ngx_buf(JSContext *cx, JSString *str, ngx_pool_t *pool, size_t len)
{
	ngx_buf_t           *b;
	const char          *p;
	
	if (len == 0)
		len = JS_GetStringLength(str);
	
	p = JS_GetStringBytes(str);
	if (p == NULL)
		return NULL;
	
	b = ngx_create_temp_buf(pool, len);
	if (b == NULL)
		return NULL;
	ngx_memcpy(b->last, p, len);
	b->last = b->last + len;
	
	return b;
}

static ngx_int_t
js_str2ngx_str(JSContext *cx, JSString *str, ngx_pool_t *pool, ngx_str_t *s, size_t len)
{
	const char          *p;
	
	s->len = 0;
	s->data = NULL;
	
	if (len == 0)
		len = JS_GetStringLength(str);
	
	if (len == 0)
		return NGX_OK;
	
	p = JS_GetStringBytes(str);
	if (p == NULL)
		return NGX_ERROR;
	
	s->data = ngx_palloc(pool, len);
	if (s->data == NULL)
		return NGX_ERROR;
	
	ngx_memcpy(s->data, p, len);
	s->len = len;
	
	return NGX_OK;
}


static JSBool
js_nginx_request_class_func_send_http_header(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
{
	ngx_http_request_t *r;
	r = (ngx_http_request_t *) JS_GetPrivate(cx, this);
	if (!r)
	{
		JS_ReportError(cx, "Trying to use Request with NULL native request pointer");
		return JS_FALSE;
	}
	
	// ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "js_nginx_request_class_func_send_http_header");
	
	if (r->headers_out.status == 0)
		r->headers_out.status = NGX_HTTP_OK;
	
	if (argc == 1)
	{
		if (!JSVAL_IS_STRING(argv[0]))
		{
			JS_ReportError(cx, "sendHttpHeader() takes one optional argument of type String");
			return JS_FALSE;
		}
		
		
		if (js_str2ngx_str(cx, JSVAL_TO_STRING(argv[0]), r->pool, &r->headers_out.content_type, 0) != NGX_OK)
		{
			JS_ReportError(cx, "Can`t js_str2ngx_str(argv[1], content_type)");
			*rval = JSVAL_FALSE;
			return JS_TRUE;
		}

		r->headers_out.content_type_len = r->headers_out.content_type.len;
    }
	
	if (ngx_http_set_content_type(r) != NGX_OK)
	{
		JS_ReportError(cx, "Can`t ngx_http_set_content_type(r)");
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}
	
	ngx_http_send_header(r);
	
	return JS_TRUE;
}


static JSBool
js_nginx_request_class_func_print_string(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
{
	ngx_http_request_t  *r;
	ngx_buf_t           *b;
	size_t               len;
	JSString            *str;
	ngx_chain_t          out;	
	
	r = (ngx_http_request_t *) JS_GetPrivate(cx, this);
	if (!r)
	{
		JS_ReportError(cx, "Trying to use Request with NULL native request pointer");
		return JS_FALSE;
	}
	
	if (argc != 1 || !JSVAL_IS_STRING(argv[0]))
	{
		JS_ReportError(cx, "Request#printString takes 1 argument of type str:String");
		return JS_FALSE;
	}
	
	str = JSVAL_TO_STRING(argv[0]);
	len = JS_GetStringLength(str);
	if (len == 0)
		return JS_TRUE;
	b = js_str2ngx_buf(cx, str, r->pool, len);
	
	
	out.buf = b;
	out.next = NULL;
	ngx_http_output_filter(r, &out);
	
	return JS_TRUE;
}

static ngx_int_t
js_nginx_request_class_func_request_handler(ngx_http_request_t *r, void *data, ngx_int_t rc)
{
	fprintf(stderr, "js_nginx_request_class_func_request_handler(%p, %p, %d)", r, data, (int)rc);
	
	if (rc == NGX_ERROR || r->connection->error || r->request_output)
		return rc;
	
	return NGX_OK;
}


static JSBool
js_nginx_request_class_func_request(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
{
	ngx_int_t                    rc;
	ngx_http_request_t          *r, *sr;
	ngx_http_post_subrequest_t  *psr;
	ngx_str_t                   *uri, args;
	ngx_uint_t                   flags;
	size_t                       len;
	JSString                    *str;
	
	
	r = (ngx_http_request_t *) JS_GetPrivate(cx, this);
	if (!r)
	{
		JS_ReportError(cx, "trying to use Request with NULL native request pointer");
		return JS_FALSE;
	}
	
	
	if (argc != 2 || !JSVAL_IS_STRING(argv[0]) || !JSVAL_IS_OBJECT(argv[1]) || !JS_ValueToFunction(cx, argv[1]))
	{
		JS_ReportError(cx, "Request#request takes 2 argument of types uri:String and callback:Function");
		return JS_FALSE;
	}
	
	str = JSVAL_TO_STRING(argv[0]);
	len = JS_GetStringLength(str);
	if (len == 0)
	{
		JS_ReportError(cx, "empty uri passed");
		return JS_FALSE;
	}
	
	uri = ngx_palloc(r->pool, sizeof(ngx_str_t));
	if (js_str2ngx_str(cx, str, r->pool, uri, len) != NGX_OK)
	{
		JS_ReportOutOfMemory(cx);
		return JS_FALSE;
	}
	
	
	psr = ngx_palloc(r->pool, sizeof(ngx_http_post_subrequest_t));
	if (psr == NULL)
	{
		// JS_ReportError(cx, "Can`t ngx_palloc()");
		JS_ReportOutOfMemory(cx);
		return JS_FALSE;
	}
	psr->handler = js_nginx_request_class_func_request_handler;
	// a callback
	psr->data = JSVAL_TO_OBJECT(argv[0]);
	
	
	
	flags = 0;
	args.len = 0;
	args.data = NULL;
	
	if (ngx_http_parse_unsafe_uri(r, uri, &args, &flags) != NGX_OK)
	{
		JS_ReportError(cx, "Error in ngx_http_parse_unsafe_uri(%s)", uri->data);
		return JS_FALSE;
	}
	flags |= NGX_HTTP_SUBREQUEST_IN_MEMORY;
	
	rc = ngx_http_subrequest(r, uri, &args, &sr, psr, flags);
	*rval = INT_TO_JSVAL(rc);
	
	return JS_TRUE;
}


static JSFunctionSpec js_nginx_request_class_funcs[] = {
    {"sendHttpHeader",    js_nginx_request_class_func_send_http_header,     1, 0, 0},
    {"printString",       js_nginx_request_class_func_print_string,         1, 0, 0},
    {"request",           js_nginx_request_class_func_request,              1, 0, 0},
    {0, NULL, 0, 0, 0}
};

#include "environment.c"

extern char **environ;

static JSObject *request_proto_obj = NULL;

static ngx_int_t
ngx_http_js_init_interpreter_nginx(ngx_conf_t *cf, ngx_http_js_main_conf_t *jsmcf, JSContext *cx, JSObject  *global)
{
	static ngx_http_js_context_private_t   private;
	JSObject *nginxobj, *envobj;
	
	if (JS_GetContextPrivate(cx))
		return NGX_ERROR;
	
	private.cf = cf;
	private.jsmcf = jsmcf;
	private.log = cf->log;
	
	JS_SetContextPrivate(cx, &private);
	
	// environment
	envobj = JS_DefineObject(cx, global, "environment", &env_class, NULL, 0);
	if (!envobj || !JS_SetPrivate(cx, envobj, environ))
		return 1;
	
	
	JS_DefineFunction(cx, global, "load", js_global_func_load, 0, 0);
	
	// Nginx
	nginxobj = JS_DefineObject(cx, global, "Nginx", &js_nginx_class, NULL, JSPROP_ENUMERATE);
	JS_DefineProperties(cx, nginxobj, js_nginx_class_props);
	JS_DefineFunctions(cx, nginxobj, js_nginx_class_funcs);
	
	// Nginx.Request
	request_proto_obj = JS_InitClass(cx, nginxobj, NULL, &js_nginx_request_class,  NULL, 0,  js_nginx_request_class_props, js_nginx_request_class_funcs,  NULL, NULL);
	if (!request_proto_obj)
	{
		ngx_log_error(NGX_LOG_ERR, cf->log, 0, "Can`t JS_InitClass(Nginx.Request)");
		return NGX_ERROR;
	}
	
	
	return NGX_OK;
}


// static ngx_int_t
// ngx_http_js_call_handler(JSContext *cx, JSObject *global, ngx_http_request_t *r, JSObject *sub, ngx_str_t *handler)
// {
// 	if (r->headers_out.status == 0)
// 		r->headers_out.status = NGX_HTTP_OK;
//     
//     if (ngx_http_set_content_type(r) != NGX_OK)
// 	{
// 		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Can`t ngx_http_set_content_type(r)");
// 		return NGX_ERROR;
// 	}
// 	
// 	ngx_http_send_header(r);
// 	
// 	return NGX_HTTP_OK;
// }

//#define unless(a) if(!(a))

static ngx_int_t
ngx_http_js_call_handler(JSContext *cx, JSObject *global, ngx_http_request_t *r, JSObject *sub, ngx_str_t *handler)
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
	
	request = JS_NewObject(cx, &js_nginx_request_class, request_proto_obj, NULL);
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

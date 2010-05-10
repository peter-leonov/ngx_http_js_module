#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>
#include <js/jsapi.h>

#include <macroses.h>

static JSBool
method_load(JSContext *cx, JSObject *self, uintN argc, jsval *argv, jsval *rval)
{
	uintN i;
	JSString *str;
	char *filename, *filevar_name;
	u_char real[NGX_MAX_PATH], *the_end;
	JSScript *script;
	JSBool ok;
	jsval result, name, old;
	uint32 oldopts;
	JSObject *global;
	// FILE *fileh;
	
	TRACE();
	
	global = JS_GetGlobalObject(cx);
	filevar_name = "__FILE__";
	
	for (i = 0; i < argc; i++)
	{
		str = JS_ValueToString(cx, argv[i]);
		if (!str)
			return JS_FALSE;
		name = argv[i] = STRING_TO_JSVAL(str);
		filename = JS_GetStringBytes(str);
		if (filename[0] != '/')
		{
			the_end = ngx_snprintf(real, NGX_MAX_PATH, "%*s/%s", ngx_cycle->conf_prefix.len, ngx_cycle->conf_prefix.data, (u_char *) filename);
			the_end[0] = '\0';
			filename = (char *) real;
		}
		ngx_log_debug1(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0, "global.load %s\n", filename);
		errno = 0;
		oldopts = JS_GetOptions(cx);
		JS_SetOptions(cx, oldopts | JSOPTION_COMPILE_N_GO);
		script = JS_CompileFile(cx, self, filename);
		if (errno)
		{
			ngx_log_debug2(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0, "error loading script %s: %s.\n", filename, strerror(errno));
		}
		// if (errno == ENOENT)
		if (!script)
			ok = JS_FALSE;
		else
		{
			JS_GetProperty(cx, global, filevar_name, &old);
			JS_SetProperty(cx, global, filevar_name, &name);
			ok = JS_ExecuteScript(cx, self, script, &result);
			JS_SetProperty(cx, global, filevar_name, &old);
		}
		JS_SetOptions(cx, oldopts);
		if (!ok)
			return JS_FALSE;
	}
	
	return JS_TRUE;
}

static JSBool
method_GC(JSContext *cx, JSObject *self, uintN argc, jsval *argv, jsval *rval)
{
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0, "JS_GC() begin");
	JS_GC(cx);
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0, "JS_GC() end");
	
	return JS_TRUE;
}

static JSBool
method_maybeGC(JSContext *cx, JSObject *self, uintN argc, jsval *argv, jsval *rval)
{
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0, "JS_MaybeGC() begin");
	JS_MaybeGC(cx);
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0, "JS_MaybeGC() end");
	
	return JS_TRUE;
}


static JSClass global_class =
{
    "global", JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};


extern char **environ;

JSBool
ngx_http_js__global__init(JSContext *cx)
{
	JSObject *global;
	
	TRACE();
	
	E(!JS_GetGlobalObject(cx), "global object already defined");
	
	E(global = JS_NewObject(cx, &global_class, NULL, NULL), "Can`t create new global object");
	
	JS_SetGlobalObject(cx, global);
	E(JS_InitStandardClasses(cx, global), "Can`t JS_InitStandardClasses()");
	E(JS_DefineProperty(cx, global, "self", OBJECT_TO_JSVAL(global), NULL, NULL, 0), "Can`t define property global.self");
	E(JS_DefineProperty(cx, global, "global", OBJECT_TO_JSVAL(global), NULL, NULL, 0), "Can`t define property global.global");
	E(JS_DefineFunction(cx, global, "load", method_load, 0, 0), "Can`t define function global.load");
	E(JS_DefineFunction(cx, global, "GC", method_GC, 0, 0), "Can`t define function global.GC");
	E(JS_DefineFunction(cx, global, "maybeGC", method_maybeGC, 0, 0), "Can`t define function global.maybeGC");
	
	return JS_TRUE;
}



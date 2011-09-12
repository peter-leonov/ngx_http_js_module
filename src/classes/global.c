#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#include <ngx_http_jsapi.h>
#include <ngx_http_js_module.h>
#include <nginx_js_glue.h>
#include <nginx_js_macroses.h>

static JSBool
method_load(JSContext *cx, uintN argc, jsval *vp)
{
	uintN i;
	JSString *str;
	char *filename, *filevar_name;
	u_char real[NGX_MAX_PATH], *the_end;
	JSBool ok;
	jsval result, name, old;
	uint32 oldopts;
	JSObject *global, *script;
	
	TRACE();
	
	global = JS_GetGlobalObject(cx);
	filevar_name = "__FILE__";
	
	for (i = 0; i < argc; i++)
	{
		str = JS_ValueToString(cx, JS_ARGV(cx, vp)[i]);
		if (!str)
		{
			return JS_FALSE;
		}
		name = STRING_TO_JSVAL(str);
		filename = JS_EncodeString(cx, str);
		if (filename == NULL)
		{
			return JS_FALSE;
		}
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
		script = JS_CompileFile(cx, global, filename);
		if (errno)
		{
			ngx_log_debug2(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0, "error loading script %s: %s.\n", filename, strerror(errno));
		}
		// if (errno == ENOENT)
		if (!script)
		{
			ok = JS_FALSE;
		}
		else
		{
			JS_GetProperty(cx, global, filevar_name, &old);
			JS_SetProperty(cx, global, filevar_name, &name);
			ok = JS_ExecuteScript(cx, global, script, &result);
			JS_SetProperty(cx, global, filevar_name, &old);
		}
		JS_SetOptions(cx, oldopts);
		JS_free(cx, filename);
		if (!ok)
		{
			return JS_FALSE;
		}
	}
	
	return JS_TRUE;
}

static JSBool
method_GC(JSContext *cx, uintN argc, jsval *vp)
{
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0, "JS_GC() begin");
	JS_GC(cx);
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0, "JS_GC() end");
	
	return JS_TRUE;
}

static JSBool
method_maybeGC(JSContext *cx, uintN argc, jsval *vp)
{
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0, "JS_MaybeGC() begin");
	JS_MaybeGC(cx);
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0, "JS_MaybeGC() end");
	
	return JS_TRUE;
}

static JSBool
getter_utf8length(JSContext *cx, JSObject *self, jsid id, jsval *vp)
{
	JSString   *str;
	size_t      len;
	
	TRACE();
	
	str = JS_ValueToString(cx, OBJECT_TO_JSVAL(self));
	if (str == NULL)
	{
		return JS_FALSE;
	}
	
	len = JS_GetStringEncodingLength(cx, str);
	if (len == (size_t) -1)
	{
		return JS_FALSE;
	}
	
	if (!JS_NewNumberValue(cx, len, vp))
	{
		return JS_FALSE;
	}
	
	return JS_TRUE;
}


static JSClass global_class =
{
	"global", JSCLASS_GLOBAL_FLAGS,
	JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
	JSCLASS_NO_OPTIONAL_MEMBERS
};


extern char **environ;

JSBool
ngx_http_js__global__init(JSContext *cx)
{
	static int  inited = 0;
	JSObject   *global, *string;
	jsval       val;
	
	TRACE();
	
	if (inited)
	{
		JS_ReportError(cx, "global object is already inited");
		return JS_FALSE;
	}
	
	E(!JS_GetGlobalObject(cx), "global object already defined");
	
	E(global = JS_NewGlobalObject(cx, &global_class), "Can`t create new global object");
	
	JS_SetGlobalObject(cx, global);
	E(JS_InitStandardClasses(cx, global), "Can`t JS_InitStandardClasses()");
	E(JS_DefineProperty(cx, global, "self", OBJECT_TO_JSVAL(global), NULL, NULL, 0), "Can`t define property global.self");
	E(JS_DefineProperty(cx, global, "global", OBJECT_TO_JSVAL(global), NULL, NULL, 0), "Can`t define property global.global");
	E(JS_DefineFunction(cx, global, "load", method_load, 0, 0), "Can`t define function global.load");
	E(JS_DefineFunction(cx, global, "GC", method_GC, 0, 0), "Can`t define function global.GC");
	E(JS_DefineFunction(cx, global, "maybeGC", method_maybeGC, 0, 0), "Can`t define function global.maybeGC");
	
	if (!JS_GetProperty(cx, global, "String", &val))
	{
		return JS_FALSE;
	}
	
	if (!JSVAL_IS_OBJECT(val))
	{
		return JS_FALSE;
	}
	
	string = JSVAL_TO_OBJECT(val);
	
	if (!JS_GetProperty(cx, string, "prototype", &val))
	{
		return JS_FALSE;
	}
	
	if (!JSVAL_IS_OBJECT(val))
	{
		return JS_FALSE;
	}
	
	string = JSVAL_TO_OBJECT(val);
	
	if (!JS_DefineProperty(cx, string, "utf8length", JSVAL_VOID, getter_utf8length, NULL, JSPROP_READONLY | JSPROP_ENUMERATE))
	{
		THROW("Can`t define property String#method_utf8length");
	}
	
	inited = 1;
	
	return JS_TRUE;
}



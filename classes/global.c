
// global class

#include <ngx_config.h>
#include <jsapi.h>

#include "../macroses.h"

#include "environment.c"

static JSBool
method_load(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
{
	uintN i;
	JSString *str;
	const char *filename, *filevar_name;
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
		LOG2("global.load %s\n", filename);
		errno = 0;
		oldopts = JS_GetOptions(cx);
		JS_SetOptions(cx, oldopts | JSOPTION_COMPILE_N_GO);
		script = JS_CompileFile(cx, this, filename);
		// LOG("Error: %i.\n", strerror(errno));
		// if (errno == ENOENT)
		if (!script)
			ok = JS_FALSE;
		else
		{
			JS_GetProperty(cx, global, filevar_name, &old);
			JS_SetProperty(cx, global, filevar_name, &name);
			ok = JS_ExecuteScript(cx, this, script, &result);
			JS_SetProperty(cx, global, filevar_name, &old);
		}
		JS_SetOptions(cx, oldopts);
		if (!ok)
			return JS_FALSE;
	}
	
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
	JSObject *envobj;
	
	TRACE();
	
	E(!JS_GetGlobalObject(cx), "global object already defined");
	
	E(global = JS_NewObject(cx, &global_class, NULL, NULL), "Can`t create new global object");
	
	JS_SetGlobalObject(cx, global);
	E(JS_InitStandardClasses(cx, global), "Can`t JS_InitStandardClasses()");
	E(JS_DefineProperty(cx, global, "self", OBJECT_TO_JSVAL(global), NULL, NULL, 0), "Can`t define property global.self");
	E(JS_DefineProperty(cx, global, "global", OBJECT_TO_JSVAL(global), NULL, NULL, 0), "Can`t define property global.global");
	E(JS_DefineFunction(cx, global, "load", method_load, 0, 0), "Can`t define function global.load");
	
	envobj = JS_DefineObject(cx, global, "environment", &env_class, NULL, 0);
	E(envobj && JS_SetPrivate(cx, envobj, environ), "Can`t define object global.environment");
	
	return JS_TRUE;
}



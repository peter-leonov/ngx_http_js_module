
// global class

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#include <js/jsapi.h>


static JSBool
global_load(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
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


static JSClass global_class =
{
    "global", JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};



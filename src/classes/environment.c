#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#include <ngx_http_jsapi.h>
#include <ngx_http_js_module.h>
#include <nginx_js_glue.h>
#include <nginx_js_macroses.h>

// token from SpiderMonkey 1.7.0 source (js.c:2740)

static JSBool
env_setProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	TRACE();
/* XXX porting may be easy, but these don't seem to supply setenv by default */
#if !defined XP_BEOS && !defined XP_OS2 && !defined SOLARIS
	JSString *idstr, *valstr;
	const char *name, *value;
	int rv;
	
	
	idstr = JS_ValueToString(cx, id);
	if (idstr == NULL)
	{
		return JS_FALSE;
	}
	
	name = JS_GetStringBytes(idstr);
	if (name[0] == '\0')
	{
		return JS_FALSE;
	}
	
	
	valstr = JS_ValueToString(cx, *vp);
	if (valstr == NULL)
	{
		return JS_FALSE;
	}
	
	value = JS_GetStringBytes(valstr);
	if (value[0] == '\0')
	{
		return JS_FALSE;
	}
	
#if defined XP_WIN || defined HPUX || defined OSF1 || defined IRIX
	{
		char *waste = JS_smprintf("%s=%s", name, value);
		if (waste == NULL)
		{
			JS_ReportOutOfMemory(cx);
			return JS_FALSE;
		}
		rv = putenv(waste);
#ifdef XP_WIN
		/*
 		 * HPUX9 at least still has the bad old non-copying putenv.
 		 *
		 * Per mail from <s.shanmuganathan@digital.com>, OSF1 also has a putenv
		 * that will crash if you pass it an auto char array (so it must place
		 * its argument directly in the char *environ[] array).
		 */
		JS_free(waste);
#endif
	}
#else
	rv = setenv(name, value, 1);
#endif
	if (rv < 0)
	{
		JS_ReportError(cx, "can't set environment variable \"%s\" to \"%s\"", name, value);
		return JS_FALSE;
	}
	*vp = STRING_TO_JSVAL(valstr);
#endif /* !defined XP_BEOS && !defined XP_OS2 && !defined SOLARIS */
	return JS_TRUE;
}

static JSBool
env_enumerate(JSContext *cx, JSObject *obj)
{
	static JSBool reflected;
	char **evp, *name, *value;
	JSString *valstr;
	
	TRACE();
	
	if (reflected)
		return JS_TRUE;
	
	for (evp = (char **) JS_GetPrivate(cx, obj); (name = *evp) != NULL; evp++)
	{
		value = strchr(name, '=');
		if (value == NULL)
		{
			continue;
		}
		
		*value++ = '\0';
		
		valstr = JS_NewStringCopyZ(cx, value);
		if (valstr == NULL)
		{
			return JS_FALSE;
		}
		
		if (!JS_DefineProperty(cx, obj, name, STRING_TO_JSVAL(valstr), NULL, NULL, JSPROP_ENUMERATE))
		{
			return JS_FALSE;
		}
		
		value[-1] = '=';
	}
	
	reflected = JS_TRUE;
	return JS_TRUE;
}

static JSBool
env_resolve(JSContext *cx, JSObject *obj, jsval id, uintN flags, JSObject **objp)
{
	JSString *idstr, *valstr;
	const char *name, *value;
	
	TRACE();
	
	if (flags & JSRESOLVE_ASSIGNING)
	{
		return JS_TRUE;
	}
	
	idstr = JS_ValueToString(cx, id);
	if (idstr == NULL)
	{
		return JS_FALSE;
	}
	
	name = JS_GetStringBytes(idstr);
	if (name[0] == '\0')
	{
		return JS_FALSE;
	}
	
	value = getenv(name);
	if (value == NULL)
	{
		return JS_TRUE;
	}
	
	valstr = JS_NewStringCopyZ(cx, value);
	if (valstr == NULL)
	{
		return JS_FALSE;
	}
	
	if (!JS_DefineProperty(cx, obj, name, STRING_TO_JSVAL(valstr), NULL, NULL, JSPROP_ENUMERATE))
	{
		return JS_FALSE;
	}
	
	*objp = obj;
	return JS_TRUE;
}

static JSClass env_class =
{
	"environment", JSCLASS_HAS_PRIVATE | JSCLASS_NEW_RESOLVE,
	JS_PropertyStub,  JS_PropertyStub,
	JS_PropertyStub,  env_setProperty,
	env_enumerate, (JSResolveOp) env_resolve,
	JS_ConvertStub,   JS_FinalizeStub,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

extern char **environ;

JSBool
ngx_http_js__environment__init(JSContext *cx, JSObject *global)
{
	JSObject *envobj;
	
	TRACE();
	
	envobj = JS_DefineObject(cx, global, "environment", &env_class, NULL, 0);
	E(envobj && JS_SetPrivate(cx, envobj, environ), "Can`t define object global.environment");
	
	return JS_TRUE;
}

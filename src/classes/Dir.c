#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#include <ngx_http_jsapi.h>

#include <ngx_http_js_module.h>
#include <nginx_js_glue.h>
#include <classes/Dir.h>

#include <nginx_js_macroses.h>

#define TRACE_METHOD() \
	ngx_log_debug2(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0, COLOR_CYAN "Dir#%s" COLOR_CLEAR "(fd=%p)", __FUNCTION__ + 7, fd);
#define TRACE_STATIC_METHOD() \
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0, COLOR_CYAN "Dir.%s" COLOR_CLEAR "()", __FUNCTION__ + 7);


JSObject *ngx_http_js__nginx_dir__prototype;
JSClass ngx_http_js__nginx_dir__class;
// static JSClass *private_class = &ngx_http_js__nginx_dir__class;


static JSBool
method_create(JSContext *cx, JSObject *self, uintN argc, jsval *argv, jsval *rval)
{
	JSString        *jss_path;
	jsdouble         dp;
	const char      *path;
	
	TRACE_STATIC_METHOD();
	
	E(argc == 2, "Nginx.Dir#create takes 2 mandatory arguments: path:String and access:Number");
	
	
	// converting smth. to a string is a very common and rather simple operation,
	// so on failure it's very likely we have gone out of memory
	
	jss_path = JS_ValueToString(cx, argv[0]);
	if (jss_path == NULL)
	{
		return JS_FALSE;
	}
	
	path = JS_GetStringBytes(jss_path);
	if (path == NULL)
	{
		return JS_FALSE;
	}
	
	
	if (!JS_ValueToNumber(cx, argv[0], &dp))
	{
		// forward exception if any
		return JS_FALSE;
	}
	
	
	ngx_log_debug2(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0, "ngx_create_dir(\"%s\", %d)", path, (int) dp);
	*rval = INT_TO_JSVAL(ngx_create_dir(path, dp));
	
	return JS_TRUE;
}


static JSBool
constructor(JSContext *cx, JSObject *self, uintN argc, jsval *argv, jsval *rval)
{
	TRACE();
	JS_SetPrivate(cx, self, NULL);
	return JS_TRUE;
}

static void
finalizer(JSContext *cx, JSObject *self)
{
	TRACE();
	JS_SetPrivate(cx, self, NULL);
}

static JSFunctionSpec funcs[] =
{
	JS_FS_END
};

static JSPropertySpec props[] =
{
	{0, 0, 0, NULL, NULL}
};


static JSFunctionSpec static_funcs[] =
{
	JS_FS("create",             method_create,               2, 0, 0),
	JS_FS_END
};

static JSPropertySpec static_props[] =
{
	{0, 0, 0, NULL, NULL}
};

JSClass ngx_http_js__nginx_dir__class =
{
	"Dir",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, finalizer,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

JSBool
ngx_http_js__nginx_dir__init(JSContext *cx, JSObject *global)
{
	static int  inited = 0;
	JSObject    *nginxobj;
	jsval        vp;
	
	if (inited)
	{
		JS_ReportError(cx, "Nginx.Dir is already inited");
		return JS_FALSE;
	}
	
	E(JS_GetProperty(cx, global, "Nginx", &vp), "global.Nginx is undefined");
	nginxobj = JSVAL_TO_OBJECT(vp);
	
	ngx_http_js__nginx_dir__prototype = JS_InitClass(cx, nginxobj, NULL, &ngx_http_js__nginx_dir__class,  constructor, 0,
		props, funcs, static_props, static_funcs);
	E(ngx_http_js__nginx_dir__prototype, "Can`t JS_InitClass(Nginx.Dir)");
	
	inited = 1;
	
	return JS_TRUE;
}

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#include <js/jsapi.h>

#include "classes/File.h"

#include "../macroses.h"

JSObject *ngx_http_js__nginx_file__prototype;
JSClass ngx_http_js__nginx_file__class;
// static JSClass *private_class = &ngx_http_js__nginx_file__class;


static JSBool
constructor(JSContext *cx, JSObject *self, uintN argc, jsval *argv, jsval *rval)
{
	TRACE();
	return JS_TRUE;
}

static JSBool
getProperty(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	TRACE();
	
	return JS_TRUE;
}

JSFunctionSpec ngx_http_js__nginx_file__funcs[] = {
    {0, NULL, 0, 0, 0}
};

JSPropertySpec ngx_http_js__nginx_file__props[] =
{
	{0, 0, 0, NULL, NULL}
};


static JSFunctionSpec static_funcs[] = {
	{0, NULL, 0, 0, 0}
};

static JSPropertySpec static_props[] =
{
	{0, 0, 0, NULL, NULL}
};

JSClass ngx_http_js__nginx_file__class =
{
	"File",
	0,
	JS_PropertyStub, JS_PropertyStub, getProperty, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

JSBool
ngx_http_js__nginx_file__init(JSContext *cx, JSObject *global)
{
	JSObject    *nginxobj;
	jsval        vp;
	
	E(JS_GetProperty(cx, global, "Nginx", &vp), "global.Nginx is undefined");
	nginxobj = JSVAL_TO_OBJECT(vp);
	
	ngx_http_js__nginx_file__prototype = JS_InitClass(cx, nginxobj, NULL, &ngx_http_js__nginx_file__class,  constructor, 0,
		ngx_http_js__nginx_file__props, ngx_http_js__nginx_file__funcs,  static_props, static_funcs);
	E(ngx_http_js__nginx_file__prototype, "Can`t JS_InitClass(Nginx.File)");
	
	return JS_TRUE;
}

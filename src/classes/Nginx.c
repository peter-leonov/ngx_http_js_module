
// Nginx class

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>
#include <ngx_md5.h>

#include <js/jsapi.h>

#include <ngx_http_js_module.h>
#include <nginx_js_glue.h>
#include <strings_util.h>
#include <nginx_js_macroses.h>

#define JS_CLASS_NAME "Nginx"

// see the http://nginx.org/pipermail/nginx-devel/2010-May/000220.html
#if (NGX_PTR_SIZE == 4)
#define JS_GET_NGINX_FULL_MSECS() ((uint64_t) ngx_cached_time->sec * 1000 + ngx_cached_time->msec)
#else
#define JS_GET_NGINX_FULL_MSECS() ngx_current_msec
#endif

static JSBool
method_logError(JSContext *cx, JSObject *self, uintN argc, jsval *argv, jsval *rval)
{
	JSString                       *jsstr;
	ngx_uint_t                      level;
	
	TRACE();
	
	if (argc != 2 || !JSVAL_IS_INT(argv[0]) || !JSVAL_IS_STRING(argv[1]))
	{
		JS_ReportError(cx, "Nginx.logError takes 2 arguments of types Integer and String");
		return JS_FALSE;
	}
	
	level = JSVAL_TO_INT(argv[0]);
	jsstr = JSVAL_TO_STRING(argv[1]);
	
	ngx_log_error(level, ngx_cycle->log, 0, "%s", JS_GetStringBytes(jsstr));
	
	return JS_TRUE;
}

static JSBool
method_md5(JSContext *cx, JSObject *self, uintN argc, jsval *argv, jsval *rval)
{
	JSString                      *jsstr;
	ngx_md5_t                      md5;
	u_char                        *str, hash[16], hex[32];
	size_t                         len;
	
	TRACE();
	
	if (argc != 1)
	{
		JS_ReportError(cx, "Nginx.md5 takes 1 argument");
		return JS_FALSE;
	}
	
	jsstr = JS_ValueToString(cx, argv[0]);
	if (jsstr == NULL)
	{
		return JS_FALSE;
	}
	
	str = (u_char *) JS_GetStringBytes(jsstr);
	if (str == NULL)
	{
		return JS_FALSE;
	}
	
	len = ngx_strlen(str);
	
	ngx_md5_init(&md5);
	ngx_md5_update(&md5, str, len);
	ngx_md5_final(hash, &md5);
	
	ngx_hex_dump(hex, hash, 16);
	
	DATA_LEN_to_JS_STRING_to_JSVAL(cx, hex, 32, *rval);
	
	return JS_TRUE;
}

static JSBool
js_nginx_class_getProperty(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
#ifdef NGX_DEBUG
	TRACE_S(js_debug_value_to_cstring(cx, id));
#else
	TRACE();
#endif
	
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
			
			// NGX_*
			case 43: *vp = INT_TO_JSVAL(NGX_OK); break;
			case 44: *vp = INT_TO_JSVAL(NGX_ERROR); break;
			case 45: *vp = INT_TO_JSVAL(NGX_AGAIN); break;
			case 46: *vp = INT_TO_JSVAL(NGX_BUSY); break;
			case 47: *vp = INT_TO_JSVAL(NGX_DONE); break;
			case 48: *vp = INT_TO_JSVAL(NGX_DECLINED); break;
			case 49: *vp = INT_TO_JSVAL(NGX_ABORT); break;
			
			// flags NGX_HTTP_
			case 50: *vp = INT_TO_JSVAL(NGX_HTTP_LAST); break;
			case 51: *vp = INT_TO_JSVAL(NGX_HTTP_FLUSH); break;
			
			
			case 100:
			{
				// ngx_time_t *tp;
				// tp = ngx_timeofday();
				// ngx_log_debug0(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0, "%L", tp->sec * 1000 + tp->msec);
				if (!JS_NewNumberValue(cx, JS_GET_NGINX_FULL_MSECS(), vp))
				{
					JS_ReportOutOfMemory(cx);
					return JS_FALSE;
				}
			}
			break;
			
			case 101:
			{
				if (!ngx_cycle->conf_prefix.len)
				{
					JS_ReportError(cx, "conf_prefix is an empty string");
					return JS_FALSE;
				}
				
				NGX_STRING_to_JS_STRING_to_JSVAL(cx, ngx_cycle->conf_prefix, *vp);
			}
			break;
			
			case 102: *vp = INT_TO_JSVAL(ngx_pid); break;
		}
	}
	return JS_TRUE;
}

static JSPropertySpec nginx_class_props[] =
{
	// NGX_LOG*
	{"LOG_STDERR",                         1,  JSPROP_READONLY, NULL, NULL},
	{"LOG_EMERG",                          2,  JSPROP_READONLY, NULL, NULL},
	{"LOG_ALERT",                          3,  JSPROP_READONLY, NULL, NULL},
	{"LOG_CRIT",                           4,  JSPROP_READONLY, NULL, NULL},
	{"LOG_ERR",                            5,  JSPROP_READONLY, NULL, NULL},
	{"LOG_WARN",                           6,  JSPROP_READONLY, NULL, NULL},
	{"LOG_NOTICE",                         7,  JSPROP_READONLY, NULL, NULL},
	{"LOG_INFO",                           8,  JSPROP_READONLY, NULL, NULL},
	{"LOG_DEBUG",                          9,  JSPROP_READONLY, NULL, NULL},
	
	// NGX_HTTP*
	{"HTTP_OK",                            10, JSPROP_READONLY, NULL, NULL},
	{"HTTP_CREATED",                       11, JSPROP_READONLY, NULL, NULL},
	{"HTTP_NO_CONTENT",                    12, JSPROP_READONLY, NULL, NULL},
	{"HTTP_PARTIAL_CONTENT",               13, JSPROP_READONLY, NULL, NULL},
	{"HTTP_SPECIAL_RESPONSE",              14, JSPROP_READONLY, NULL, NULL},
	{"HTTP_MOVED_PERMANENTLY",             15, JSPROP_READONLY, NULL, NULL},
	{"HTTP_MOVED_TEMPORARILY",             16, JSPROP_READONLY, NULL, NULL},
	{"HTTP_NOT_MODIFIED",                  17, JSPROP_READONLY, NULL, NULL},
	{"HTTP_BAD_REQUEST",                   18, JSPROP_READONLY, NULL, NULL},
	{"HTTP_UNAUTHORIZED",                  19, JSPROP_READONLY, NULL, NULL},
	{"HTTP_FORBIDDEN",                     20, JSPROP_READONLY, NULL, NULL},
	{"HTTP_NOT_FOUND",                     21, JSPROP_READONLY, NULL, NULL},
	{"HTTP_NOT_ALLOWED",                   22, JSPROP_READONLY, NULL, NULL},
	{"HTTP_REQUEST_TIME_OUT",              23, JSPROP_READONLY, NULL, NULL},
	{"HTTP_CONFLICT",                      24, JSPROP_READONLY, NULL, NULL},
	{"HTTP_LENGTH_REQUIRED",               25, JSPROP_READONLY, NULL, NULL},
	{"HTTP_PRECONDITION_FAILED",           26, JSPROP_READONLY, NULL, NULL},
	{"HTTP_REQUEST_ENTITY_TOO_LARGE",      27, JSPROP_READONLY, NULL, NULL},
	{"HTTP_REQUEST_URI_TOO_LARGE",         28, JSPROP_READONLY, NULL, NULL},
	{"HTTP_UNSUPPORTED_MEDIA_TYPE",        29, JSPROP_READONLY, NULL, NULL},
	{"HTTP_RANGE_NOT_SATISFIABLE",         30, JSPROP_READONLY, NULL, NULL},
	{"HTTP_CLOSE",                         31, JSPROP_READONLY, NULL, NULL},
	{"HTTP_OWN_CODES",                     32, JSPROP_READONLY, NULL, NULL},
	{"HTTPS_CERT_ERROR",                   33, JSPROP_READONLY, NULL, NULL},
	{"HTTPS_NO_CERT",                      34, JSPROP_READONLY, NULL, NULL},
	{"HTTP_TO_HTTPS",                      35, JSPROP_READONLY, NULL, NULL},
	{"HTTP_CLIENT_CLOSED_REQUEST",         36, JSPROP_READONLY, NULL, NULL},
	{"HTTP_INTERNAL_SERVER_ERROR",         37, JSPROP_READONLY, NULL, NULL},
	{"HTTP_NOT_IMPLEMENTED",               38, JSPROP_READONLY, NULL, NULL},
	{"HTTP_BAD_GATEWAY",                   39, JSPROP_READONLY, NULL, NULL},
	{"HTTP_SERVICE_UNAVAILABLE",           40, JSPROP_READONLY, NULL, NULL},
	{"HTTP_GATEWAY_TIME_OUT",              41, JSPROP_READONLY, NULL, NULL},
	{"HTTP_INSUFFICIENT_STORAGE",          42, JSPROP_READONLY, NULL, NULL},
	
	// NGX_*
	{"OK",                                 43, JSPROP_READONLY, NULL, NULL},
	{"ERROR",                              44, JSPROP_READONLY, NULL, NULL},
	{"AGAIN",                              45, JSPROP_READONLY, NULL, NULL},
	{"BUSY",                               46, JSPROP_READONLY, NULL, NULL},
	{"DONE",                               47, JSPROP_READONLY, NULL, NULL},
	{"DECLINED",                           48, JSPROP_READONLY, NULL, NULL},
	{"ABORT",                              49, JSPROP_READONLY, NULL, NULL},
	
	// flags NGX_HTTP_
	{"HTTP_LAST",                          50, JSPROP_READONLY, NULL, NULL},
	{"HTTP_FLUSH",                         51, JSPROP_READONLY, NULL, NULL},
	
	{"time",                              100, JSPROP_READONLY, NULL, NULL},
	{"prefix",                            101, JSPROP_READONLY, NULL, NULL},
	{"pid",                               102, JSPROP_READONLY, NULL, NULL},
	
	{0, 0, 0, NULL, NULL}
};

static JSClass nginx_class =
{
	"Nginx",
	0,
	JS_PropertyStub, JS_PropertyStub, js_nginx_class_getProperty, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSFunctionSpec nginx_class_funcs[] =
{
	{"logError",    method_logError,         1, 0, 0},
	{"md5",         method_md5,              1, 0, 0},
	{0, NULL, 0, 0, 0}
};

JSBool
ngx_http_js__nginx__init(JSContext *cx, JSObject *global)
{
	JSObject *nginxobj;
	
	TRACE();
	
	nginxobj = JS_DefineObject(cx, global, "Nginx", &nginx_class, NULL, JSPROP_ENUMERATE);
	if (!nginxobj)
	{
		JS_ReportError(cx, "Can`t JS_DefineObject(Nginx)");
		return JS_FALSE;
	}
	
	JS_DefineProperties(cx, nginxobj, nginx_class_props);
	JS_DefineFunctions(cx, nginxobj, nginx_class_funcs);
	
	return JS_TRUE;
}

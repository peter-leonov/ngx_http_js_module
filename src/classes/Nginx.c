
// Nginx class

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>
#include <ngx_md5.h>

#include <ngx_http_jsapi.h>

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
	
	ngx_log_error(level, js_log(), 0, "%s", JS_GetStringBytes(jsstr));
	
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

#define SET_INT(name, val) \
v = INT_TO_JSVAL(val); \
if (!JS_SetProperty(cx, obj, name, &v)) \
{ \
	return JS_FALSE; \
}

#define SET_CST(name, val) \
v_jss = JS_NewStringCopyZ(cx, val); \
if (v_jss == NULL) \
{ \
	return JS_FALSE; \
} \
v = STRING_TO_JSVAL(v_jss); \
if (!JS_SetProperty(cx, obj, name, &v)) \
{ \
	return JS_FALSE; \
}

static JSBool
setup_constants(JSContext *cx, JSObject *obj)
{
	jsval      v;
	JSString  *v_jss;
	
	// NGX_LOG_*
	SET_INT("LOG_STDERR",                      NGX_LOG_STDERR);
	SET_INT("LOG_EMERG",                       NGX_LOG_EMERG);
	SET_INT("LOG_ALERT",                       NGX_LOG_ALERT);
	SET_INT("LOG_CRIT",                        NGX_LOG_CRIT);
	SET_INT("LOG_ERR",                         NGX_LOG_ERR);
	SET_INT("LOG_WARN",                        NGX_LOG_WARN);
	SET_INT("LOG_NOTICE",                      NGX_LOG_NOTICE);
	SET_INT("LOG_INFO",                        NGX_LOG_INFO);
	SET_INT("LOG_DEBUG",                       NGX_LOG_DEBUG);
	
	// NGX_HTTP_*
	SET_INT("HTTP_OK",                         NGX_HTTP_OK);
	SET_INT("HTTP_CREATED",                    NGX_HTTP_CREATED);
	SET_INT("HTTP_NO_CONTENT",                 NGX_HTTP_NO_CONTENT);
	SET_INT("HTTP_PARTIAL_CONTENT",            NGX_HTTP_PARTIAL_CONTENT);
	SET_INT("HTTP_SPECIAL_RESPONSE",           NGX_HTTP_SPECIAL_RESPONSE);
	SET_INT("HTTP_MOVED_PERMANENTLY",          NGX_HTTP_MOVED_PERMANENTLY);
	SET_INT("HTTP_MOVED_TEMPORARILY",          NGX_HTTP_MOVED_TEMPORARILY);
	SET_INT("HTTP_NOT_MODIFIED",               NGX_HTTP_NOT_MODIFIED);
	SET_INT("HTTP_BAD_REQUEST",                NGX_HTTP_BAD_REQUEST);
	SET_INT("HTTP_UNAUTHORIZED",               NGX_HTTP_UNAUTHORIZED);
	SET_INT("HTTP_FORBIDDEN",                  NGX_HTTP_FORBIDDEN);
	SET_INT("HTTP_NOT_FOUND",                  NGX_HTTP_NOT_FOUND);
	SET_INT("HTTP_NOT_ALLOWED",                NGX_HTTP_NOT_ALLOWED);
	SET_INT("HTTP_REQUEST_TIME_OUT",           NGX_HTTP_REQUEST_TIME_OUT);
	SET_INT("HTTP_CONFLICT",                   NGX_HTTP_CONFLICT);
	SET_INT("HTTP_LENGTH_REQUIRED",            NGX_HTTP_LENGTH_REQUIRED);
	SET_INT("HTTP_PRECONDITION_FAILED",        NGX_HTTP_PRECONDITION_FAILED);
	SET_INT("HTTP_REQUEST_ENTITY_TOO_LARGE",   NGX_HTTP_REQUEST_ENTITY_TOO_LARGE);
	SET_INT("HTTP_REQUEST_URI_TOO_LARGE",      NGX_HTTP_REQUEST_URI_TOO_LARGE);
	SET_INT("HTTP_UNSUPPORTED_MEDIA_TYPE",     NGX_HTTP_UNSUPPORTED_MEDIA_TYPE);
	SET_INT("HTTP_RANGE_NOT_SATISFIABLE",      NGX_HTTP_RANGE_NOT_SATISFIABLE);
	SET_INT("HTTP_CLOSE",                      NGX_HTTP_CLOSE);
	SET_INT("HTTP_OWN_CODES",                  NGX_HTTP_OWN_CODES);
	SET_INT("HTTPS_CERT_ERROR",                NGX_HTTPS_CERT_ERROR);
	SET_INT("HTTPS_NO_CERT",                   NGX_HTTPS_NO_CERT);
	SET_INT("HTTP_TO_HTTPS",                   NGX_HTTP_TO_HTTPS);
	SET_INT("HTTP_CLIENT_CLOSED_REQUEST",      NGX_HTTP_CLIENT_CLOSED_REQUEST);
	SET_INT("HTTP_INTERNAL_SERVER_ERROR",      NGX_HTTP_INTERNAL_SERVER_ERROR);
	SET_INT("HTTP_NOT_IMPLEMENTED",            NGX_HTTP_NOT_IMPLEMENTED);
	SET_INT("HTTP_BAD_GATEWAY",                NGX_HTTP_BAD_GATEWAY);
	SET_INT("HTTP_SERVICE_UNAVAILABLE",        NGX_HTTP_SERVICE_UNAVAILABLE);
	SET_INT("HTTP_GATEWAY_TIME_OUT",           NGX_HTTP_GATEWAY_TIME_OUT);
	SET_INT("HTTP_INSUFFICIENT_STORAGE",       NGX_HTTP_INSUFFICIENT_STORAGE);
	
	// NGX_*
	SET_INT("OK",                              NGX_OK);
	SET_INT("ERROR",                           NGX_ERROR);
	SET_INT("AGAIN",                           NGX_AGAIN);
	SET_INT("BUSY",                            NGX_BUSY);
	SET_INT("DONE",                            NGX_DONE);
	SET_INT("DECLINED",                        NGX_DECLINED);
	SET_INT("ABORT",                           NGX_ABORT);
	
	// flags NGX_HTTP_*
	SET_INT("HTTP_LAST",                       NGX_HTTP_LAST);
	SET_INT("HTTP_FLUSH",                      NGX_HTTP_FLUSH);
	
	// etc
	SET_INT("version",                         nginx_version);
	SET_CST("VERSION",                         NGINX_VERSION);
	return JS_TRUE;
}


static JSBool
getter_time(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	TRACE_STATIC_GETTER()
	
	if (!JS_NewNumberValue(cx, JS_GET_NGINX_FULL_MSECS(), vp))
	{
		JS_ReportOutOfMemory(cx);
		return JS_FALSE;
	}
	
	return JS_TRUE;
}


static JSBool
getter_prefix(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	TRACE_STATIC_GETTER()
	
	if (!ngx_cycle->conf_prefix.len)
	{
		JS_ReportError(cx, "ngx_cycle->conf_prefix is an empty string");
		return JS_FALSE;
	}
	
	NGX_STRING_to_JS_STRING_to_JSVAL(cx, ngx_cycle->conf_prefix, *vp);
	
	
	return JS_TRUE;
}


static JSBool
getter_pid(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	TRACE_STATIC_GETTER()
	
	*vp = INT_TO_JSVAL(ngx_pid);
	
	return JS_TRUE;
}


static JSPropertySpec nginx_class_props[] =
{
	{"time",                                0, JSPROP_READONLY, getter_time,   NULL},
	{"prefix",                            101, JSPROP_READONLY, getter_prefix, NULL},
	{"pid",                               102, JSPROP_READONLY, getter_pid,    NULL},
	
	{0, 0, 0, NULL, NULL}
};

static JSClass nginx_class =
{
	"Nginx",
	0,
	JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSFunctionSpec nginx_class_funcs[] =
{
	JS_FS("logError",    method_logError,         1, 0, 0),
	JS_FS("md5",         method_md5,              1, 0, 0),
	JS_FS_END
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
	
	setup_constants(cx, nginxobj);
	
	JS_DefineProperties(cx, nginxobj, nginx_class_props);
	JS_DefineFunctions(cx, nginxobj, nginx_class_funcs);
	
	return JS_TRUE;
}

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#include <ngx_http_jsapi.h>

#include <ngx_http_js_module.h>
#include <strings_util.h>
#include <classes/Request.h>

#include <nginx_js_macroses.h>


JSObject *ngx_http_js__nginx_cookies__prototype;
JSClass ngx_http_js__nginx_cookies__class;
static JSClass* private_class = &ngx_http_js__nginx_cookies__class;

static ngx_int_t
multi_parts_count(ngx_array_t *headers);


JSObject *
ngx_http_js__nginx_cookies__wrap(JSContext *cx, ngx_http_request_t *r)
{
	JSObject                  *cookies;
	ngx_http_js_ctx_t         *ctx;
	
	TRACE();
	
	if (!(ctx = ngx_http_get_module_ctx(r, ngx_http_js_module)))
		return NULL;
	
	if (ctx->js_cookies != NULL)
		return ctx->js_cookies;
	
	cookies = JS_NewObject(cx, &ngx_http_js__nginx_cookies__class, ngx_http_js__nginx_cookies__prototype, NULL);
	if (!cookies)
	{
		JS_ReportOutOfMemory(cx);
		return NULL;
	}
	
	if (!JS_SetReservedSlot(cx, ctx->js_request, NGX_JS_REQUEST_SLOT__COOKIES, OBJECT_TO_JSVAL(cookies)))
	{
		JS_ReportError(cx, "can't set slot NGX_JS_REQUEST_SLOT__COOKIES(%d)", NGX_JS_REQUEST_SLOT__COOKIES);
		return NULL;
	}
	
	JS_SetPrivate(cx, cookies, r);
	
	ctx->js_cookies = cookies;
	
	return cookies;
}


void
ngx_http_js__nginx_cookies__cleanup(ngx_http_js_ctx_t *ctx, ngx_http_request_t *r, JSContext *cx)
{
	ngx_assert(ctx);
	
	if (ctx->js_cookies == NULL)
		return;
	
	JS_SetPrivate(cx, ctx->js_cookies, NULL);
	ctx->js_cookies = NULL;
}

static JSBool
method_empty(JSContext *cx, JSObject *self, uintN argc, jsval *argv, jsval *rval)
{
	ngx_http_request_t         *r;
	
	TRACE();
	GET_PRIVATE(r);
	
	r->headers_in.cookies.nelts = 0;
	
	return JS_TRUE;
}

static JSBool
constructor(JSContext *cx, JSObject *self, uintN argc, jsval *argv, jsval *rval)
{
	TRACE();
	JS_ReportError(cx, "new Nginx.Cookie() can be constucted only with request#headersIn#cookies");
	return JS_FALSE;
}


// enum propid { HEADER_LENGTH };


static JSBool
getProperty(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	
	TRACE();
	GET_PRIVATE(r);
	
	if (JSVAL_IS_STRING(id))
	{
		char        *name;
		ngx_int_t    n;
		ngx_str_t    cookie_name, cookie_value;
		
		name = JS_GetStringBytes(JSVAL_TO_STRING(id));
		if (name == NULL)
		{
			return JS_FALSE;
		}
		
		ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "cookies[\"%s\"]", name);
		
		cookie_name.data = (u_char *) name;
		cookie_name.len = ngx_strlen(name);
		
		n = ngx_http_parse_multi_header_lines(&r->headers_in.cookies, &cookie_name, &cookie_value);
		
		if (n == NGX_DECLINED)
		{
			return JS_TRUE;
		}
		
		NGX_STRING_to_JS_STRING_to_JSVAL(cx, cookie_value, *vp);
	}
	else if (JSVAL_IS_INT(id))
	{
		switch (JSVAL_TO_INT(id))
		{
			case 1:
			{
				*vp = INT_TO_JSVAL(multi_parts_count(&r->headers_in.cookies));
			}
			break;
		}
	}
	
	return JS_TRUE;
}


static JSPropertySpec ngx_http_js__nginx_cookies__props[] =
{
	{"length",                 1,          JSPROP_READONLY,   NULL, NULL},
	{0, 0, 0, NULL, NULL}
};


static JSFunctionSpec ngx_http_js__nginx_cookies__funcs[] =
{
	JS_FS("empty",       method_empty,          0, 0, 0),
	JS_FS_END
};

JSClass ngx_http_js__nginx_cookies__class =
{
	"Cookies",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub, getProperty, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

JSBool
ngx_http_js__nginx_cookies__init(JSContext *cx, JSObject *global)
{
	JSObject    *nginxobj;
	jsval        vp;
	
	E(JS_GetProperty(cx, global, "Nginx", &vp), "global.Nginx is undefined");
	nginxobj = JSVAL_TO_OBJECT(vp);
	
	ngx_http_js__nginx_cookies__prototype = JS_InitClass(cx, nginxobj, NULL, &ngx_http_js__nginx_cookies__class,  constructor, 0,
		ngx_http_js__nginx_cookies__props, ngx_http_js__nginx_cookies__funcs,  NULL, NULL);
	E(ngx_http_js__nginx_cookies__prototype, "Can`t JS_InitClass(Nginx.Cookies)");
	
	return JS_TRUE;
}


static ngx_int_t
multi_parts_count(ngx_array_t *headers)
{
	ngx_uint_t         i;
	size_t             count = 0;
	u_char            *start, *end;
	ngx_table_elt_t  **h;
	
	h = headers->elts;
	
	for (i = 0; i < headers->nelts; i++)
	{
		start = h[i]->value.data;
		end = h[i]->value.data + h[i]->value.len;
		
		while (start < end)
		{
			if (*start++ == '=')
			{
				count++;
			}
		}
	}
	
	return count;
}


// Nginx.Request class

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#include <js/jsapi.h>

#include "../ngx_http_js_module.h"
#include "../strings_util.h"
#include "../macroses.h"


JSObject *ngx_http_js__nginx_request_prototype;
JSClass ngx_http_js__nginx_request_class;



static JSBool
method_sendHttpHeader(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
{
	ngx_http_request_t *r;
	GET_PRIVATE();
	
	LOG("Nginx.Request#sendHttpHeader");
	
	if (r->headers_out.status == 0)
		r->headers_out.status = NGX_HTTP_OK;
	
	if (argc == 1)
	{
		E(JSVAL_IS_STRING(argv[0]), "sendHttpHeader() takes one optional argument: contentType:String");
		
		E(js_str2ngx_str(cx, JSVAL_TO_STRING(argv[0]), r->pool, &r->headers_out.content_type, 0),
			"Can`t js_str2ngx_str(cx, contentType)")
		
		r->headers_out.content_type_len = r->headers_out.content_type.len;
    }
	
	E(ngx_http_set_content_type(r) == NGX_OK, "Can`t ngx_http_set_content_type(r)")
	E(ngx_http_send_header(r) == NGX_OK, "Can`t ngx_http_send_header(r)");
	
	*rval = JSVAL_TRUE;
	return JS_TRUE;
}


static JSBool
method_printString(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
{
	LOG("Nginx.Request#printString");
	ngx_http_request_t  *r;
	ngx_buf_t           *b;
	size_t               len;
	JSString            *str;
	ngx_chain_t          out;	
	
	GET_PRIVATE();
	
	E(argc == 1 && JSVAL_IS_STRING(argv[0]), "Nginx.Request#printString takes 1 argument: str:String");
	
	str = JSVAL_TO_STRING(argv[0]);
	len = JS_GetStringLength(str);
	if (len == 0)
		return JS_TRUE;
	b = js_str2ngx_buf(cx, str, r->pool, len);
	
	
	out.buf = b;
	out.next = NULL;
	ngx_http_output_filter(r, &out);
	
	return JS_TRUE;
}

static ngx_int_t
method_request_handler(ngx_http_request_t *r, void *data, ngx_int_t rc)
{
	
	LOG("method_request_handler(%p, %p, %d)", r, data, (int)rc);
	
	ngx_http_js_context_private_t    *private;
	ngx_http_js_ctx_t                *ctx;
	JSObject                         *request, *func;
	JSContext                        *cx;
	jsval                             rval;//, req;
	
	func = data;
	
	ctx = ngx_http_get_module_ctx(r->main, ngx_http_js_module);
	
	if (!ctx)
		return NGX_ERROR;
	
	request = ctx->js_request;
	cx = ctx->js_cx;
	
	if (!func || !request || !cx)
		return NGX_ERROR;
	
	// LOG("data = %p", data);
	// LOG("cx = %p", cx);
	// LOG("request = %p", request);
	
	
	
	private = JS_GetContextPrivate(cx);
	if (!JS_ObjectIsFunction(cx, func))
	{
		ngx_log_error(NGX_LOG_ERR, private->log, 0, "callback is not a function");
		return NGX_ERROR;
	}
	
	// req = OBJECT_TO_JSVAL(request);
	JS_CallFunctionValue(cx, request, OBJECT_TO_JSVAL(func), 0, NULL, &rval);
	
	if (rc == NGX_ERROR || r->connection->error || r->request_output)
		return rc;
	
	
	return NGX_OK;
}


static JSBool
method_request(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
{
	ngx_int_t                    rc;
	ngx_http_js_ctx_t           *ctx;
	ngx_http_request_t          *r, *sr;
	ngx_http_post_subrequest_t  *psr;
	ngx_str_t                   *uri, args;
	ngx_uint_t                   flags;
	size_t                       len;
	JSString                    *str;
	JSObject                    *request;
	
	
	GET_PRIVATE();
	
	E(argc == 2 && JSVAL_IS_STRING(argv[0]) && JSVAL_IS_OBJECT(argv[1]) && JS_ValueToFunction(cx, argv[1]),
		"Request#request takes 2 arguments: uri:String and callback:Function");
	
	str = JSVAL_TO_STRING(argv[0]);
	len = JS_GetStringLength(str);
	E(len, "empty uri passed");
	
	E(uri = ngx_palloc(r->pool, sizeof(ngx_str_t)), "Can`t ngx_palloc(...)");
	E(js_str2ngx_str(cx, str, r->pool, uri, len), "Can`t js_str2ngx_str(...)")
	
	
	E(psr = ngx_palloc(r->pool, sizeof(ngx_http_post_subrequest_t)), "Can`t ngx_palloc()");
	psr->handler = method_request_handler;
	// a js-callback
	psr->data = JSVAL_TO_OBJECT(argv[1]);
	
	// LOG("psr->data = %p", psr->data);
	// LOG("cx = %p", cx);
	// LOG("request = %p", this);
	
	flags = 0;
	args.len = 0;
	args.data = NULL;
	
	E(ngx_http_parse_unsafe_uri(r, uri, &args, &flags) == NGX_OK, "Error in ngx_http_parse_unsafe_uri(%s)", uri->data)
	
	flags |= NGX_HTTP_SUBREQUEST_IN_MEMORY;
	
	rc = ngx_http_subrequest(r, uri, &args, &sr, psr, flags);
	sr->filter_need_in_memory = 1;
	
	E(request = JS_NewObject(cx, &ngx_http_js__nginx_request_class, ngx_http_js__nginx_request_prototype, NULL), "Can`t JS_NewObject()");
	
	ctx = ngx_http_get_module_ctx(sr, ngx_http_js_module);
	// ctx->js_request = request;
	// ctx->js_cx = cx;
	
	*rval = INT_TO_JSVAL(rc);
	return JS_TRUE;
}


enum request_propid { REQUEST_URI, REQUEST_METHOD, REQUEST_REMOTE_ADDR };

static JSBool
request_getProperty(JSContext *cx, JSObject *this, jsval id, jsval *vp)
{
	ngx_http_request_t *r;
	
	GET_PRIVATE();
	
	// LOG("Nginx.Request property id = %d\n", JSVAL_TO_INT(id));
	if (JSVAL_IS_INT(id))
	{
		switch (JSVAL_TO_INT(id))
		{
			case REQUEST_URI:
			*vp = STRING_TO_JSVAL(JS_NewStringCopyN(cx, (char *) r->uri.data, r->uri.len));
			break;
			
			case REQUEST_METHOD:
			*vp = STRING_TO_JSVAL(JS_NewStringCopyN(cx, (char *) r->method_name.data, r->method_name.len));
			break;
			
			case REQUEST_REMOTE_ADDR:
			*vp = STRING_TO_JSVAL(JS_NewStringCopyN(cx, (char *) r->connection->addr_text.data, r->connection->addr_text.len));
			break;
			
		}
	}
	return JS_TRUE;
}

JSPropertySpec ngx_http_js__nginx_request_props[] =
{
	{"uri",      REQUEST_URI,          JSPROP_READONLY,   NULL, NULL},
	// {"discardRequestBody",       MY_COLOR,       JSPROP_ENUMERATE,  NULL, NULL},
	{"method",   REQUEST_METHOD,       JSPROP_READONLY,   NULL, NULL},
	// {"headerOnly",       MY_COLOR,       JSPROP_ENUMERATE,  NULL, NULL},
	{"remoteAddr",      REQUEST_REMOTE_ADDR,      JSPROP_READONLY,  NULL, NULL},
	// {"status",       MY_WIDTH,       JSPROP_ENUMERATE,  NULL, NULL},
	// {"requestBody",       MY_FUNNY,       JSPROP_ENUMERATE,  NULL, NULL},
	// {"requestBodyFile",       MY_ARRAY,       JSPROP_ENUMERATE,  NULL, NULL},
	// {"allowRanges",       MY_ARRAY,       JSPROP_ENUMERATE,  NULL, NULL},
	// {"headerOnly",      MY_RDONLY,      JSPROP_READONLY,   NULL, NULL},
	{0, 0, 0, NULL, NULL}
};


JSFunctionSpec ngx_http_js__nginx_request_funcs[] = {
    {"sendHttpHeader",    method_sendHttpHeader,     1, 0, 0},
    {"printString",       method_printString,         1, 0, 0},
    {"request",           method_request,              1, 0, 0},
    {0, NULL, 0, 0, 0}
};

JSClass ngx_http_js__nginx_request_class =
{
	"Request",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub, request_getProperty, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

JSBool
ngx_http_js__nginx_request__init(JSContext *cx)
{
	JSObject    *nginxobj;
	JSObject    *global;
	jsval        vp;
	
	global = JS_GetGlobalObject(cx);
	
	E(JS_GetProperty(cx, global, "Nginx", &vp), "global.Nginx is undefined or is not a function");
	nginxobj = JSVAL_TO_OBJECT(vp);
	
	ngx_http_js__nginx_request_prototype = JS_InitClass(cx, nginxobj, NULL, &ngx_http_js__nginx_request_class,  NULL, 0,
		ngx_http_js__nginx_request_props, ngx_http_js__nginx_request_funcs,  NULL, NULL);
	E(ngx_http_js__nginx_request_prototype, "Can`t JS_InitClass(Nginx.Request)");
	
	return JS_TRUE;
}

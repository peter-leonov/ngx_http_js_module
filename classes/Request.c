
// Nginx.Request class

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#include <js/jsapi.h>
#include <assert.h>

#include "../ngx_http_js_module.h"
#include "../strings_util.h"
#include "../macroses.h"

//#define unless(a) if(!(a))
#define JS_REQUEST_ROOT_NAME          "Nginx.Request instance"
#define JS_REQUEST_CALLBACK_ROOT_NAME      "Nginx.Request callback function"


// static JSObject *requests_cache_array;
JSObject *ngx_http_js__nginx_request_prototype;
JSClass ngx_http_js__nginx_request_class;

static void
cleanup_handler(void *data);


JSObject *
ngx_http_js__wrap_nginx_request(JSContext *cx, ngx_http_request_t *r)
{
	LOG2("ngx_http_js__wrap_nginx_request(%p, %p)", cx, r);
	
	JSObject                  *request;
	ngx_http_js_ctx_t         *ctx;
	ngx_http_cleanup_t        *cln;
	// jsuint                     length;
	// jsval                      req;
	
	if ((ctx = ngx_http_get_module_ctx(r, ngx_http_js_module)))
	{
		if (!ctx->js_cx)
			ctx->js_cx = cx;
		if (ctx->js_request)
			return ctx->js_request;
	}
	else
	{
		ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_js_ctx_t));
		if (ctx == NULL)
		{
			JS_ReportOutOfMemory(cx);
			return NULL;
		}
		
		ngx_http_set_ctx(r, ctx, ngx_http_js_module);
	}
	
	
	request = JS_NewObject(cx, &ngx_http_js__nginx_request_class, ngx_http_js__nginx_request_prototype, NULL);
	if (!request)
	{
		JS_ReportOutOfMemory(cx);
		return NULL;
	}
	
	if (!JS_AddNamedRoot(cx, &ctx->js_request, JS_REQUEST_ROOT_NAME))
	{
		JS_ReportError(cx, "Can`t add new root %s", JS_REQUEST_ROOT_NAME);
		return NULL;
	}
	
	// if (!JS_GetArrayLength(cx, requests_cache_array, &length))
	// {
	// 	JS_ReportError(cx, "Can`t get __requests_cache lenght");
	// 	return NULL;
	// }
	// 
	// req = OBJECT_TO_JSVAL(request);
	// if (!JS_SetElement(cx, requests_cache_array, length, &req))
	// {
	// 	JS_ReportError(cx, "Can`t set __requests_cache %u-th element", length);
	// 	return NULL;
	// }
	
	cln = ngx_http_cleanup_add(r, 0);
	cln->data = r;
	cln->handler = cleanup_handler;
	
	JS_SetPrivate(cx, request, r);
	
	ctx->js_request = request;
	ctx->js_cx = cx;
	
	return request;
}



static void
cleanup_handler(void *data)
{
	ngx_http_request_t        *r;
	ngx_http_js_ctx_t         *ctx;
	JSContext                 *cx;
	JSObject                  *request;
	jsval                      rval;
	
	r = data;
	
	ctx = ngx_http_get_module_ctx(r, ngx_http_js_module);
	assert(ctx);
	cx = ctx->js_cx;
	request = ctx->js_request;
	assert(cx && request);
	
	// LOG("cleanup");
	if (!JS_CallFunctionName(cx, request, "cleanup", 0, NULL, &rval))
		JS_ReportError(cx, "Error calling Nginx.Request#cleanup");
	
	// second param has to be &ctx->js_request
	// because JS_AddRoot was used with it's address
	if (!JS_RemoveRoot(cx, &ctx->js_request))
		JS_ReportError(cx, "Can`t remove cleaned up root %s", JS_REQUEST_ROOT_NAME);
	
	if (ctx->js_callback)
		if (!JS_RemoveRoot(cx, &ctx->js_callback))
			JS_ReportError(cx, "Can`t remove cleaned up root %s", JS_REQUEST_CALLBACK_ROOT_NAME);
	
	// finaly mark the object as inactive
	// after that the GET_PRIVATE macros will throw an exception when is called 
	JS_SetPrivate(cx, request, NULL);
}


static JSBool
method_cleanup(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
{
	return JS_TRUE;
}


static JSBool
method_sendHttpHeader(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
{
	ngx_http_request_t *r;
	GET_PRIVATE();
	
	LOG2("Nginx.Request#sendHttpHeader");
	
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
	LOG2("Nginx.Request#printString");
	ngx_http_request_t  *r;
	ngx_buf_t           *b;
	size_t               len;
	JSString            *str;
	ngx_chain_t          out;
	ngx_int_t            rc;
	
	GET_PRIVATE();
	
	E(argc == 1 && JSVAL_IS_STRING(argv[0]), "Nginx.Request#printString takes 1 argument: str:String");
	
	str = JSVAL_TO_STRING(argv[0]);
	len = JS_GetStringLength(str);
	if (len == 0)
		return JS_TRUE;
	b = js_str2ngx_buf(cx, str, r->pool, len);
	
	
	out.buf = b;
	out.next = NULL;
	rc = ngx_http_output_filter(r, &out);
	
	*rval = INT_TO_JSVAL(rc);
	
	return JS_TRUE;
}


static JSBool
method_sendSpecial(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
{
	LOG2("Nginx.Request#sendSpecial");
	ngx_http_request_t  *r;
	ngx_int_t            rc;
	
	GET_PRIVATE();
	
	E(argc == 1 && JSVAL_IS_INT(argv[0]), "Nginx.Request#sendSpecial takes 1 argument: flags:Number");
	
	rc = ngx_http_send_special(r, (ngx_uint_t)JSVAL_TO_INT(argv[0]));
	*rval = INT_TO_JSVAL(rc);
	return JS_TRUE;
}

static ngx_int_t
method_request_handler(ngx_http_request_t *sr, void *data, ngx_int_t rc)
{
	LOG2("method_request_handler(%p, %p, %d)", sr, data, (int)rc);
	
	ngx_http_js_ctx_t                *ctx, *mctx;
	JSObject                         *request, *subrequest, *func;
	// JSString                         *body;
	JSContext                        *cx;
	jsval                             rval, args[2];
	
	if (rc == NGX_ERROR || sr->connection->error || sr->request_output)
		return rc;
	
	// LOG("sr in callback = %p", sr);
	ctx = ngx_http_get_module_ctx(sr, ngx_http_js_module);
	if (!ctx)
		return NGX_ERROR;
	
	cx = ctx->js_cx;
	assert(cx);
	subrequest = ngx_http_js__wrap_nginx_request(cx, sr);
	assert(subrequest);
	func = data;
	assert(func);
	
	mctx = ngx_http_get_module_ctx(sr->main, ngx_http_js_module);
	assert(mctx);
	request = mctx->js_request;
	assert(request);
	
	// LOG("data = %p", data);
	// LOG("cx = %p", cx);
	// LOG("request = %p", request);
	
	// LOG("sr->upstream = %p", sr->upstream);
	// LOG("sr->upstream = %s", sr->upstream->buffer.pos);
	
	if (sr->upstream)
		args[1] = STRING_TO_JSVAL(JS_NewStringCopyN(cx, (char*) sr->upstream->buffer.pos, sr->upstream->buffer.last-sr->upstream->buffer.pos));
	else
		args[1] = JSVAL_VOID;
	
	
	if (!JS_ObjectIsFunction(cx, func))
	{
		JS_ReportError(cx, "subrequest callback is not a function");
		return NGX_ERROR;
	}
	
	args[0] = OBJECT_TO_JSVAL(subrequest);
	JS_CallFunctionValue(cx, request, OBJECT_TO_JSVAL(func), 2, args, &rval);
	
	return NGX_OK;
}


static JSBool
method_request(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
{
	LOG2("method_request(...)");
	
	ngx_int_t                    rc;
	ngx_http_js_ctx_t           *ctx;
	ngx_http_request_t          *r, *sr;
	ngx_http_post_subrequest_t  *psr;
	ngx_str_t                   *uri, args;
	ngx_uint_t                   flags;
	size_t                       len;
	JSString                    *str;
	JSObject                    *callback;
	
	
	GET_PRIVATE();
	
	// LOG("argc = %u", argc);
	E((argc == 1 && JSVAL_IS_STRING(argv[0]))
		|| (argc == 2 && JSVAL_IS_STRING(argv[0]) && JSVAL_IS_OBJECT(argv[1]) && JS_ValueToFunction(cx, argv[1])),
		"Request#request takes 1 argument: uri:String; and 1 optional callback:Function");
	
	str = JSVAL_TO_STRING(argv[0]);
	len = JS_GetStringLength(str);
	E(len, "empty uri passed");
	
	E(uri = ngx_palloc(r->pool, sizeof(ngx_str_t)), "Can`t ngx_palloc(...)");
	E(js_str2ngx_str(cx, str, r->pool, uri, len), "Can`t js_str2ngx_str(...)")
	
	
	flags = 0;
	args.len = 0;
	args.data = NULL;
	
	E(ngx_http_parse_unsafe_uri(r, uri, &args, &flags) == NGX_OK, "Error in ngx_http_parse_unsafe_uri(%s)", uri->data)
	
	psr = NULL;
	if (argc == 2)
	{
		callback = JSVAL_TO_OBJECT(argv[1]);
		assert(callback);
		
		E(psr = ngx_palloc(r->pool, sizeof(ngx_http_post_subrequest_t)), "Can`t ngx_palloc()");
		psr->handler = method_request_handler;
		psr->data = callback;
		
		flags |= NGX_HTTP_SUBREQUEST_IN_MEMORY;
	}
	
	
	sr = NULL;
	// LOG("before");
	// if subrequest is finished quickly, the callback will be called immediately
	rc = ngx_http_subrequest(r, uri, &args, &sr, psr, flags);
	// LOG("after");
	if (sr == NULL || rc == NGX_ERROR)
	{
		JS_ReportError(cx, "Can`t ngx_http_subrequest(...)");
		return JS_FALSE;
	}
	// sr->filter_need_in_memory = 1;
	
	if (argc == 2)
	{
		assert(sr);
		ngx_http_js__wrap_nginx_request(cx, sr);
		ctx = ngx_http_get_module_ctx(sr, ngx_http_js_module);
		assert(ctx);
		// this helps to prevent wrong JS garbage collection
		ctx->js_callback = psr->data;
		E(JS_AddNamedRoot(cx, &ctx->js_callback, JS_REQUEST_CALLBACK_ROOT_NAME), "Can`t add new root %s", JS_REQUEST_CALLBACK_ROOT_NAME);
	}
	
	// LOG("sr in request() = %p", sr);
	
	// request = ngx_http_js__wrap_nginx_request(cx, sr);
	// if (request == NULL)
	// {
	// 	ngx_http_finalize_request(sr, NGX_ERROR);
	// 	return NGX_ERROR;
	// }
	
	*rval = INT_TO_JSVAL(rc);
	return JS_TRUE;
}


static JSBool
request_constructor(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
{
	return JS_TRUE;
}


enum request_propid { REQUEST_URI, REQUEST_METHOD, REQUEST_REMOTE_ADDR };

static JSBool
request_getProperty(JSContext *cx, JSObject *this, jsval id, jsval *vp)
{
	ngx_http_request_t *r;
	
	// LOG("Nginx.Request property id = %d\n", JSVAL_TO_INT(id));
	// JS_ReportError(cx, "Nginx.Request property id = %d\n", JSVAL_TO_INT(id));
	
	GET_PRIVATE();
	
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
    {"sendHttpHeader",    method_sendHttpHeader,       0, 0, 0},
    {"printString",       method_printString,          1, 0, 0},
    {"request",           method_request,              2, 0, 0},
    {"cleanup",           method_cleanup,              0, 0, 0},
    {"sendSpecial",       method_sendSpecial,          1, 0, 0},
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
	
	// E(requests_cache_array = JS_NewArrayObject(cx, 0, NULL), "Can`t create array for global.__requests_cache");
	
	
	E(JS_GetProperty(cx, global, "Nginx", &vp), "global.Nginx is undefined or is not a function");
	nginxobj = JSVAL_TO_OBJECT(vp);
	
	ngx_http_js__nginx_request_prototype = JS_InitClass(cx, nginxobj, NULL, &ngx_http_js__nginx_request_class,  request_constructor, 0,
		ngx_http_js__nginx_request_props, ngx_http_js__nginx_request_funcs,  NULL, NULL);
	E(ngx_http_js__nginx_request_prototype, "Can`t JS_InitClass(Nginx.Request)");
	
	return JS_TRUE;
}

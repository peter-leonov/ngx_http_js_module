
// Nginx.Request class

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#include <js/jsapi.h>


enum request_propid { REQUEST_URI, REQUEST_METHOD, REQUEST_REMOTE_ADDR };

static JSBool
request_getProperty(JSContext *cx, JSObject *this, jsval id, jsval *vp)
{
	ngx_http_request_t *r;
	r = (ngx_http_request_t *) JS_GetPrivate(cx, this);
	if (!r)
	{
		JS_ReportError(cx, "Trying to use a Request instance with a NULL native request pointer");
		return JS_FALSE;
	}
	
	// fprintf(stderr, "Nginx.Request property id = %d\n", JSVAL_TO_INT(id));
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

static JSClass ngx_http_js_nginx_request_class =
{
	"Request",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub, request_getProperty, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

JSPropertySpec ngx_http_js_nginx_request_props[] =
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


static JSBool
method_sendHttpHeader(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
{
	ngx_http_request_t *r;
	r = (ngx_http_request_t *) JS_GetPrivate(cx, this);
	if (!r)
	{
		JS_ReportError(cx, "Trying to use Request with NULL native request pointer");
		return JS_FALSE;
	}
	
	// ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "method_sendHttpHeader");
	
	if (r->headers_out.status == 0)
		r->headers_out.status = NGX_HTTP_OK;
	
	if (argc == 1)
	{
		if (!JSVAL_IS_STRING(argv[0]))
		{
			JS_ReportError(cx, "sendHttpHeader() takes one optional argument of type String");
			return JS_FALSE;
		}
		
		
		if (js_str2ngx_str(cx, JSVAL_TO_STRING(argv[0]), r->pool, &r->headers_out.content_type, 0) != NGX_OK)
		{
			JS_ReportError(cx, "Can`t js_str2ngx_str(argv[1], content_type)");
			*rval = JSVAL_FALSE;
			return JS_TRUE;
		}

		r->headers_out.content_type_len = r->headers_out.content_type.len;
    }
	
	if (ngx_http_set_content_type(r) != NGX_OK)
	{
		JS_ReportError(cx, "Can`t ngx_http_set_content_type(r)");
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}
	
	ngx_http_send_header(r);
	
	return JS_TRUE;
}


static JSBool
method_printString(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
{
	ngx_http_request_t  *r;
	ngx_buf_t           *b;
	size_t               len;
	JSString            *str;
	ngx_chain_t          out;	
	
	r = (ngx_http_request_t *) JS_GetPrivate(cx, this);
	if (!r)
	{
		JS_ReportError(cx, "Trying to use Request with NULL native request pointer");
		return JS_FALSE;
	}
	
	if (argc != 1 || !JSVAL_IS_STRING(argv[0]))
	{
		JS_ReportError(cx, "Request#printString takes 1 argument of type str:String");
		return JS_FALSE;
	}
	
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
	fprintf(stderr, "method_request_handler(%p, %p, %d)", r, data, (int)rc);
	
	if (rc == NGX_ERROR || r->connection->error || r->request_output)
		return rc;
	
	return NGX_OK;
}


static JSBool
method_request(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
{
	ngx_int_t                    rc;
	ngx_http_request_t          *r, *sr;
	ngx_http_post_subrequest_t  *psr;
	ngx_str_t                   *uri, args;
	ngx_uint_t                   flags;
	size_t                       len;
	JSString                    *str;
	
	
	r = (ngx_http_request_t *) JS_GetPrivate(cx, this);
	if (!r)
	{
		JS_ReportError(cx, "trying to use Request with NULL native request pointer");
		return JS_FALSE;
	}
	
	
	if (argc != 2 || !JSVAL_IS_STRING(argv[0]) || !JSVAL_IS_OBJECT(argv[1]) || !JS_ValueToFunction(cx, argv[1]))
	{
		JS_ReportError(cx, "Request#request takes 2 argument of types uri:String and callback:Function");
		return JS_FALSE;
	}
	
	str = JSVAL_TO_STRING(argv[0]);
	len = JS_GetStringLength(str);
	if (len == 0)
	{
		JS_ReportError(cx, "empty uri passed");
		return JS_FALSE;
	}
	
	uri = ngx_palloc(r->pool, sizeof(ngx_str_t));
	if (js_str2ngx_str(cx, str, r->pool, uri, len) != NGX_OK)
	{
		JS_ReportOutOfMemory(cx);
		return JS_FALSE;
	}
	
	
	psr = ngx_palloc(r->pool, sizeof(ngx_http_post_subrequest_t));
	if (psr == NULL)
	{
		// JS_ReportError(cx, "Can`t ngx_palloc()");
		JS_ReportOutOfMemory(cx);
		return JS_FALSE;
	}
	psr->handler = method_request_handler;
	// a callback
	psr->data = JSVAL_TO_OBJECT(argv[0]);
	
	
	
	flags = 0;
	args.len = 0;
	args.data = NULL;
	
	if (ngx_http_parse_unsafe_uri(r, uri, &args, &flags) != NGX_OK)
	{
		JS_ReportError(cx, "Error in ngx_http_parse_unsafe_uri(%s)", uri->data);
		return JS_FALSE;
	}
	flags |= NGX_HTTP_SUBREQUEST_IN_MEMORY;
	
	rc = ngx_http_subrequest(r, uri, &args, &sr, psr, flags);
	*rval = INT_TO_JSVAL(rc);
	
	return JS_TRUE;
}


static JSFunctionSpec js_nginx_request_class_funcs[] = {
    {"sendHttpHeader",    method_sendHttpHeader,     1, 0, 0},
    {"printString",       method_printString,         1, 0, 0},
    {"request",           method_request,              1, 0, 0},
    {0, NULL, 0, 0, 0}
};
	// static JSObject *ngx_http_js__request_prototype = NULL;
	// // Nginx.Request
	// ngx_http_js__request_prototype = JS_InitClass(cx, nginxobj, NULL, &js_nginx_request_class,  NULL, 0,  js_nginx_request_class_props, js_nginx_request_class_funcs,  NULL, NULL);
	// if (!ngx_http_js__request_prototype)
	// {
	// 	ngx_log_error(NGX_LOG_ERR, cf->log, 0, "Can`t JS_InitClass(Nginx.Request)");
	// 	return NGX_ERROR;
	// }


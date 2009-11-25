
// Nginx.Request class

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#include <jsapi.h>

#include "../ngx_http_js_module.h"
#include "../strings_util.h"
#include "HeadersIn.h"
#include "HeadersOut.h"
#include "Chain.h"

#include "../macroses.h"

#define JS_REQUEST_ROOT_NAME               "Nginx.Request instance"
#define JS_REQUEST_CALLBACK_ROOT_NAME      "Nginx.Request subreuest callback function"
#define JS_HAS_BODY_CALLBACK_ROOT_NAME     "Nginx.Request hasBody callback function"
#define JS_SET_TIMEOUT_CALLBACK_ROOT_NAME     "Nginx.Request setTimeout callback function"


JSObject *ngx_http_js__nginx_request__prototype;
JSClass ngx_http_js__nginx_request__class;
static JSClass *private_class = &ngx_http_js__nginx_request__class;

static void
cleanup_handler(void *data);

static void
method_setTimeout_handler(ngx_event_t *ev);


JSObject *
ngx_http_js__nginx_request__wrap(JSContext *cx, ngx_http_request_t *r)
{
	JSObject                  *request;
	ngx_http_js_ctx_t         *ctx;
	ngx_http_cleanup_t        *cln;
	
	ngx_assert(cx);
	ngx_assert(r);
	TRACE_REQUEST("request_wrap");
	
	if ((ctx = ngx_http_get_module_ctx(r, ngx_http_js_module)))
	{
		if (!ctx->js_cx)
			ctx->js_cx = cx;
		if (ctx->js_request)
			return ctx->js_request;
	}
	else
	{
		// ngx_pcalloc fills allocated memory with zeroes
		ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_js_ctx_t));
		if (ctx == NULL)
		{
			JS_ReportOutOfMemory(cx);
			return NULL;
		}
		
		ngx_http_set_ctx(r, ctx, ngx_http_js_module);
		ctx->js_timer.timer_set = 0;
	}
	
	
	request = JS_NewObject(cx, &ngx_http_js__nginx_request__class, ngx_http_js__nginx_request__prototype, NULL);
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
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "js request cleanup_handler(r=%p)", r);
	
	ctx = ngx_http_get_module_ctx(r, ngx_http_js_module);
	cx = ctx->js_cx;
	request = ctx->js_request;
	ngx_assert(cx);
	ngx_assert(request);
	
	// LOG("cleanup");
	if (!JS_CallFunctionName(cx, request, "cleanup", 0, NULL, &rval))
		JS_ReportError(cx, "Error calling Nginx.Request#cleanup");
	
	// let the Headers modules to deside what to clean up
	ngx_http_js__nginx_headers_in__cleanup(cx, r, ctx);
	ngx_http_js__nginx_headers_out__cleanup(cx, r, ctx);
	
	// second param has to be &ctx->js_request
	// because JS_AddRoot was used with it's address
	if (!JS_RemoveRoot(cx, &ctx->js_request))
		JS_ReportError(cx, "Can`t remove cleaned up root %s", JS_REQUEST_ROOT_NAME);
	
	
	// ensure roots deleted to prevent memory leaks
	if (ctx->js_request_callback)
		if (!JS_RemoveRoot(cx, &ctx->js_request_callback))
			JS_ReportError(cx, "Can`t remove cleaned up root %s", JS_REQUEST_CALLBACK_ROOT_NAME);
	
	if (ctx->js_has_body_callback)
		if (!JS_RemoveRoot(cx, &ctx->js_has_body_callback))
			JS_ReportError(cx, "Can`t remove cleaned up root %s", JS_HAS_BODY_CALLBACK_ROOT_NAME);
	
	if (ctx->js_set_timeout_callback)
		if (!JS_RemoveRoot(cx, &ctx->js_set_timeout_callback))
			JS_ReportError(cx, "Can`t remove cleaned up root %s", JS_SET_TIMEOUT_CALLBACK_ROOT_NAME);
	
	
	if (ctx->js_timer.timer_set)
	{
		ngx_del_timer(&ctx->js_timer);
		ctx->js_timer.timer_set = 0;
	}
	
	// finaly mark the object as inactive
	// after that the GET_PRIVATE macros will raise an exception when called
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
	
	GET_PRIVATE(r);
	TRACE_REQUEST_METHOD();
	
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
	ngx_http_request_t  *r;
	ngx_buf_t           *b;
	size_t               len;
	JSString            *str;
	ngx_chain_t          out;
	ngx_int_t            rc;
	
	GET_PRIVATE(r);
	TRACE_REQUEST_METHOD();
	
	E(argc == 1 && JSVAL_IS_STRING(argv[0]), "Nginx.Request#printString takes 1 argument: str:String");
	
	str = JSVAL_TO_STRING(argv[0]);
	len = JS_GetStringLength(str);
	if (len == 0)
		return JS_TRUE;
	b = js_str2ngx_buf(cx, str, r->pool, len);
	
	ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "js printing string \"%*s\"", len > 25 ? 25 : len , b->last - len);
	
	out.buf = b;
	out.next = NULL;
	rc = ngx_http_output_filter(r, &out);
	
	*rval = INT_TO_JSVAL(rc);
	return JS_TRUE;
}


static JSBool
method_flush(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
{
	ngx_http_request_t  *r;
	ngx_buf_t           *b;
	ngx_chain_t          out;
	ngx_int_t            rc;
	
	GET_PRIVATE(r);
	TRACE_REQUEST_METHOD();
	
	b = ngx_calloc_buf(r->pool);
	if (b == NULL)
	{
		JS_ReportOutOfMemory(cx);
		return JS_FALSE;
	}
	b->flush = 1;
	
	out.buf = b;
	out.next = NULL;
	rc = ngx_http_output_filter(r, &out);
	
	*rval = INT_TO_JSVAL(rc);
	return JS_TRUE;
}


static JSBool
method_nextBodyFilter(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
{
	ngx_http_request_t  *r;
	ngx_buf_t           *b;
	size_t               len;
	JSString            *str;
	ngx_chain_t          out, *ch;
	ngx_int_t            rc;
	
	GET_PRIVATE(r);
	TRACE_REQUEST_METHOD();
	
	E(ngx_http_js_next_body_filter != NULL, "ngx_http_js_next_body_filter is NULL at this moment");
	
	if (argc == 1 && JSVAL_IS_STRING(argv[0]))
	{
		str = JSVAL_TO_STRING(argv[0]);
		len = JS_GetStringLength(str);
		if (len == 0)
			return JS_TRUE;
		b = js_str2ngx_buf(cx, str, r->pool, len);
		b->last_buf = 1;
	
		out.buf = b;
		out.next = NULL;
		rc = ngx_http_js_next_body_filter(r, &out);
	}
	else if (argc == 1 && JSVAL_IS_OBJECT(argv[0]))
	{
		if ( (ch = JS_GetInstancePrivate(cx, JSVAL_TO_OBJECT(argv[0]), &ngx_http_js__nginx_chain__class, NULL)) == NULL )
		{
			JS_ReportError(cx, "second parameter is object but not a chain or chain has NULL private pointer");
			return JS_FALSE;
		}
		
		rc = ngx_http_js_next_body_filter(r, ch);
	}
	else if (argc == 0 || (argc == 1 && JSVAL_IS_VOID(argv[0])))
		rc = ngx_http_js_next_body_filter(r, NULL);
	else
		E(0, "Nginx.Request#nextBodyFilter takes 1 optional argument: str:(String|undefined)");
	
	*rval = INT_TO_JSVAL(rc);
	
	return JS_TRUE;
}


static JSBool
method_sendString(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
{
	ngx_http_request_t  *r;
	ngx_buf_t           *b;
	size_t               len;
	JSString            *str;
	ngx_chain_t          out;
	ngx_int_t            rc;
	
	GET_PRIVATE(r);
	TRACE_REQUEST_METHOD();
	
	E((argc == 1 || argc == 2),
		"Nginx.Request#sendString takes 1 mandatory argument: str:String, and 1 optional: contentType:String");
	
	str = JS_ValueToString(cx, argv[0]);
	len = JS_GetStringLength(str);
	if (len == 0)
		return JS_TRUE;
	b = js_str2ngx_buf(cx, str, r->pool, len);
	if (b == NULL)
	{
		JS_ReportOutOfMemory(cx);
		return JS_FALSE;
	}
	ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "js sending string \"%*s\"", len > 25 ? 25 : len , b->last - len);
	
	ngx_http_clear_content_length(r);
	r->headers_out.content_length_n = len;
	
	if (r->headers_out.status == 0)
		r->headers_out.status = NGX_HTTP_OK;
	
	if (argc == 2)
	{
		E(js_str2ngx_str(cx, JS_ValueToString(cx, argv[1]), r->pool, &r->headers_out.content_type, 0),
			"Can`t js_str2ngx_str(cx, contentType)")
		
		r->headers_out.content_type_len = r->headers_out.content_type.len;
    }
	
	E(ngx_http_set_content_type(r) == NGX_OK, "Can`t ngx_http_set_content_type(r)")
	E(ngx_http_send_header(r) == NGX_OK, "Can`t ngx_http_send_header(r)");
	
	out.buf = b;
	out.next = NULL;
	rc = ngx_http_output_filter(r, &out);
	
	ngx_http_send_special(r, NGX_HTTP_FLUSH);
	
	*rval = INT_TO_JSVAL(rc);
	return JS_TRUE;
}


static JSBool
method_sendSpecial(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
{
	ngx_http_request_t  *r;
	
	GET_PRIVATE(r);
	TRACE_REQUEST_METHOD();
	
	E(argc == 1 && JSVAL_IS_INT(argv[0]), "Nginx.Request#sendSpecial takes 1 argument: flags:Number");
	
	*rval = INT_TO_JSVAL(ngx_http_send_special(r, (ngx_uint_t)JSVAL_TO_INT(argv[0])));
	return JS_TRUE;
}

void
method_hasBody_handler(ngx_http_request_t *r);

static JSBool
method_hasBody(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
{
	ngx_http_request_t  *r;
	ngx_http_js_ctx_t   *ctx;
	
	GET_PRIVATE(r);
	TRACE_REQUEST_METHOD();
	
	E(argc == 1 && JSVAL_IS_OBJECT(argv[0]) && JS_ValueToFunction(cx, argv[0]), "Request#hasBody takes 1 argument: callback:Function");
	
	
	if (r->headers_in.content_length_n <= 0)
	{
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}
	
	ctx = ngx_http_get_module_ctx(r, ngx_http_js_module);
	ngx_assert(ctx);
	
	ctx->js_has_body_callback = JSVAL_TO_OBJECT(argv[0]);
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "set has_body_callback to %p", ctx->js_has_body_callback);
	E(JS_AddNamedRoot(cx, &ctx->js_has_body_callback, JS_HAS_BODY_CALLBACK_ROOT_NAME), "Can`t add new root %s", JS_REQUEST_CALLBACK_ROOT_NAME);
	
	r->request_body_in_single_buf = 1;
	r->request_body_in_persistent_file = 1;
	r->request_body_in_clean_file = 1;
	
	if (r->request_body_in_file_only)
		r->request_body_file_log_level = 0;
	
	*rval = INT_TO_JSVAL(ngx_http_read_client_request_body(r, method_hasBody_handler));
	return JS_TRUE;
}

void
method_hasBody_handler(ngx_http_request_t *r)
{
	ngx_http_js_ctx_t                *ctx;
	JSObject                         *request, *callback;
	JSContext                        *cx;
	jsval                             rval;
	
	TRACE_REQUEST("hasBody handler");
	
	// if (r->connection->error)
	// 	return;
	
	ctx = ngx_http_get_module_ctx(r, ngx_http_js_module);
	if (!ctx)
		return;
	
	cx = ctx->js_cx;
	ngx_assert(cx);
	request = ngx_http_js__nginx_request__wrap(cx, r);
	ngx_assert(request);
	callback = ctx->js_has_body_callback;
	ngx_assert(callback);
	
	if (!JS_ObjectIsFunction(cx, callback))
	{
		JS_ReportError(cx, "hasBody callback is not a function");
		return;
	}
	
	JS_CallFunctionValue(cx, request, OBJECT_TO_JSVAL(callback), 0, NULL, &rval);
}


static JSBool
method_discardBody(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
{
	ngx_http_request_t  *r;
	
	GET_PRIVATE(r);
	TRACE_REQUEST_METHOD();
	
	*rval = INT_TO_JSVAL(ngx_http_discard_request_body(r));
	return JS_TRUE;
}


static JSBool
method_sendfile(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
{
	ngx_http_request_t  *r;
	char                      *filename;
	int                        offset;
	size_t                     bytes;
	ngx_str_t                  path;
	ngx_buf_t                 *b;
	ngx_open_file_info_t       of;
	ngx_http_core_loc_conf_t  *clcf;
	ngx_chain_t                out;
	// ngx_int_t            rc;
	
	GET_PRIVATE(r);
	TRACE_REQUEST_METHOD();
	
	
	E(argc >= 1 && JSVAL_IS_STRING(argv[0]),
		"Nginx.Request#sendfile takes 1 mandatory argument: filename:String, and 2 optional offset:Number and bytes:Number");
	
	
	E(js_str2c_str(cx, JSVAL_TO_STRING(argv[0]), r->pool, &filename, NULL), "Can`t js_str2c_str()");
	ngx_assert(filename);
	
	offset = argc < 2 ? -1 : JSVAL_TO_INT(argv[1]);
	bytes = argc < 3 ? 0 : JSVAL_TO_INT(argv[2]);
	
	ngx_log_debug3(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "sending file \"%s\" with offset=%d and bytes=%d", filename, offset, bytes);
	
	b = ngx_calloc_buf(r->pool);
	E(b != NULL, "Can`t ngx_calloc_buf()");
	
	b->file = ngx_pcalloc(r->pool, sizeof(ngx_file_t));
	E(b->file != NULL, "Can`t ngx_pcalloc()");
	
	clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);
	ngx_assert(clcf);
	
	
	of.test_dir = 0;
	of.valid = clcf->open_file_cache_valid;
	of.min_uses = clcf->open_file_cache_min_uses;
	of.errors = clcf->open_file_cache_errors;
	of.events = clcf->open_file_cache_events;
	
	path.len = ngx_strlen(filename);
	
	path.data = ngx_pcalloc(r->pool, path.len + 1);
	E(path.data != NULL, "Can`t ngx_pcalloc()");
	
	(void) ngx_cpystrn(path.data, (u_char*)filename, path.len + 1);
	
	if (ngx_open_cached_file(clcf->open_file_cache, &path, &of, r->pool) != NGX_OK)
	{
		if (of.err != 0)
			ngx_log_error(NGX_LOG_CRIT, r->connection->log, ngx_errno, ngx_open_file_n " \"%s\" failed", filename);
		*rval = JS_FALSE;
		return JS_TRUE;
	}
	
	if (offset == -1)
		offset = 0;
	
	if (bytes == 0)
		bytes = of.size - offset;
	
	b->in_file = 1;
	
	b->file_pos = offset;
	b->file_last = offset + bytes;
	
	b->file->fd = of.fd;
	b->file->log = r->connection->log;
	
	
	out.buf = b;
	out.next = NULL;
	
	*rval = INT_TO_JSVAL(ngx_http_output_filter(r, &out));
	return JS_TRUE;
}


static JSBool
method_setTimeout(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
{
	ngx_http_request_t  *r;
	ngx_http_js_ctx_t   *ctx;
	ngx_event_t         *timer;
	JSObject            *callback;
	
	GET_PRIVATE(r);
	TRACE_REQUEST_METHOD();
	
	E(argc >= 1 && argc <= 2 && JSVAL_IS_OBJECT(argv[0]) && JS_ObjectIsFunction(cx, callback = JSVAL_TO_OBJECT(argv[0])) && (argc == 1 || JSVAL_IS_INT(argv[1])),
			"Nginx.Request#setTimeout() takes one mandatory argument callback:Function and one optional milliseconds:Number");
	
	
	ctx = ngx_http_get_module_ctx(r, ngx_http_js_module);
	ngx_assert(ctx);
	timer = &ctx->js_timer;
	
	// E(timer->timer_set != 0, "only one timer may be set an once");
	
	
	if (ctx->js_set_timeout_callback)
	{
		if (!JS_RemoveRoot(cx, &ctx->js_set_timeout_callback))
		{
			JS_ReportError(cx, "Can`t remove cleaned up root %s", JS_SET_TIMEOUT_CALLBACK_ROOT_NAME);
			return JS_FALSE;
		}
		ctx->js_set_timeout_callback = NULL;
	}
	
	ctx->js_set_timeout_callback = callback;
	E(JS_AddNamedRoot(cx, &ctx->js_set_timeout_callback, JS_SET_TIMEOUT_CALLBACK_ROOT_NAME), "Can`t add new root %s", JS_SET_TIMEOUT_CALLBACK_ROOT_NAME);
	
	// from ngx_cycle.c:740
	timer->handler = method_setTimeout_handler;
	timer->log = r->connection->log;
	timer->data = r;
	
	
	r->main->count++;
	ngx_add_timer(timer, argc == 2 ? (ngx_uint_t) argv[1] : 0);
	timer->timer_set = 1;
	
	
	return JS_TRUE;
}

static void
method_setTimeout_handler(ngx_event_t *timer)
{
	ngx_http_request_t  *r;
	ngx_int_t            rc;
	ngx_http_js_ctx_t   *ctx;
	JSContext           *cx;
	jsval                rval;//, args[2];
	JSObject            *callback;
	
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0, "setTimeout handler");
	
	r = timer->data;
	
	ctx = ngx_http_get_module_ctx(r, ngx_http_js_module);
	ngx_assert(ctx);
	
	
	cx = ctx->js_cx;
	
	if (ctx->js_set_timeout_callback)
	{
		if (JS_ObjectIsFunction(cx, ctx->js_set_timeout_callback))
		{
			// preserve current callback
			callback = ctx->js_set_timeout_callback;
			
			// free current callback (TODO: check if GC may occur and destroy freed calback just here)
			if (!JS_RemoveRoot(cx, &ctx->js_set_timeout_callback))
				JS_ReportError(cx, "Can`t remove cleaned up root %s", JS_SET_TIMEOUT_CALLBACK_ROOT_NAME);
			ctx->js_set_timeout_callback = NULL;
			
			// here a new timeout handler may be set
			if (JS_CallFunctionValue(cx, ctx->js_request, OBJECT_TO_JSVAL(callback), 0, NULL, &rval))
				rc = NGX_OK;
			else
				rc = NGX_HTTP_INTERNAL_SERVER_ERROR;
		}
		else
		{
			ngx_log_error(NGX_LOG_CRIT, r->connection->log, 0, "setTimeout callback is not a function");
			rc = NGX_ERROR;
		}
	}
	else
	{
		ngx_log_error(NGX_LOG_CRIT, r->connection->log, 0, "setTimeout handler called with NULL callback");
		rc = NGX_ERROR;
	}
	
	ngx_http_finalize_request(r, rc);
}



static ngx_int_t
method_subrequest_handler(ngx_http_request_t *sr, void *data, ngx_int_t rc);

static JSBool
method_subrequest(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
{
	ngx_int_t                    rc;
	ngx_http_js_ctx_t           *ctx;
	ngx_http_request_t          *r, *sr;
	ngx_http_post_subrequest_t  *psr;
	ngx_str_t                   *uri, args;
	ngx_uint_t                   flags;
	size_t                       len;
	JSString                    *str;
	JSObject                    *callback;
	
	GET_PRIVATE(r);
	TRACE_REQUEST_METHOD();
	
	// LOG("argc = %u", argc);
	E((argc == 1 && JSVAL_IS_STRING(argv[0]))
		|| (argc == 2 && JSVAL_IS_STRING(argv[0]) && JSVAL_IS_OBJECT(argv[1]) && JS_ValueToFunction(cx, argv[1])),
		"Request#subrequest takes 1 argument: uri:String; and 1 optional callback:Function");
	
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
		ngx_assert(callback);
		
		E(psr = ngx_palloc(r->pool, sizeof(ngx_http_post_subrequest_t)), "Can`t ngx_palloc()");
		psr->handler = method_subrequest_handler;
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
		ngx_assert(sr);
		ngx_http_js__nginx_request__wrap(cx, sr);
		ctx = ngx_http_get_module_ctx(sr, ngx_http_js_module);
		ngx_assert(ctx);
		// this helps to prevent wrong JS garbage collection
		ctx->js_request_callback = psr->data;
		E(JS_AddNamedRoot(cx, &ctx->js_request_callback, JS_REQUEST_CALLBACK_ROOT_NAME), "Can`t add new root %s", JS_REQUEST_CALLBACK_ROOT_NAME);
	}
	
	// LOG("sr in request() = %p", sr);
	
	// request = ngx_http_js__nginx_request__wrap(cx, sr);
	// if (request == NULL)
	// {
	// 	ngx_http_finalize_request(sr, NGX_ERROR);
	// 	return NGX_ERROR;
	// }
	
	*rval = INT_TO_JSVAL(rc);
	return JS_TRUE;
}

static ngx_int_t
method_subrequest_handler(ngx_http_request_t *sr, void *data, ngx_int_t rc)
{
	ngx_http_js_ctx_t                *ctx, *mctx;
	JSObject                         *request, *subrequest, *callback;
	// JSString                         *body;
	JSContext                        *cx;
	jsval                             rval, args[2];
	
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, sr->connection->log, 0, "subrequest handler");
	
	ngx_assert(sr);
	if (rc == NGX_ERROR || sr->connection->error || sr->request_output)
		return rc;
	
	ctx = ngx_http_get_module_ctx(sr, ngx_http_js_module);
	if (!ctx)
		return NGX_ERROR;
	
	cx = ctx->js_cx;
	ngx_assert(cx);
	subrequest = ngx_http_js__nginx_request__wrap(cx, sr);
	ngx_assert(subrequest);
	callback = data;
	ngx_assert(callback);
	
	mctx = ngx_http_get_module_ctx(sr->main, ngx_http_js_module);
	ngx_assert(mctx);
	request = mctx->js_request;
	ngx_assert(request);
	
	// LOG("data = %p", data);
	// LOG("cx = %p", cx);
	// LOG("request = %p", request);
	
	// LOG("sr->upstream = %p", sr->upstream);
	// LOG("sr->upstream = %s", sr->upstream->buffer.pos);
	
	if (sr->upstream)
		args[1] = STRING_TO_JSVAL(JS_NewStringCopyN(cx, (char*) sr->upstream->buffer.pos, sr->upstream->buffer.last-sr->upstream->buffer.pos));
	else
		args[1] = JSVAL_VOID;
	
	
	if (!JS_ObjectIsFunction(cx, callback))
	{
		JS_ReportError(cx, "subrequest callback is not a function");
		return NGX_ERROR;
	}
	
	args[0] = OBJECT_TO_JSVAL(subrequest);
	JS_CallFunctionValue(cx, request, OBJECT_TO_JSVAL(callback), 2, args, &rval);
	
	return NGX_OK;
}


static JSBool
request_constructor(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
{
	return JS_TRUE;
}


enum request_propid
{
	REQUEST_URI, REQUEST_METHOD, REQUEST_FILENAME, REQUEST_REMOTE_ADDR, REQUEST_ARGS,
	REQUEST_HEADERS_IN, REQUEST_HEADERS_OUT,
	REQUEST_HEADER_ONLY, REQUEST_BODY_FILENAME, REQUEST_BODY
};

static JSBool
request_getProperty(JSContext *cx, JSObject *this, jsval id, jsval *vp)
{
	ngx_http_request_t   *r;
	JSObject             *headers;
	// LOG("Nginx.Request property id = %d\n", JSVAL_TO_INT(id));
	// JS_ReportError(cx, "Nginx.Request property id = %d\n", JSVAL_TO_INT(id));
	
	GET_PRIVATE(r);
	
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
			
			case REQUEST_FILENAME:
			{
				size_t root;
				ngx_http_js_ctx_t  *ctx;
				
				ctx = ngx_http_get_module_ctx(r, ngx_http_js_module);
				ngx_assert(ctx);
				if (!ctx->filename.data)
				{
					if (ngx_http_map_uri_to_path(r, &ctx->filename, &root, 0) == NULL)
						break;
					ctx->filename.len--;
				}
				
				*vp = STRING_TO_JSVAL(JS_NewStringCopyN(cx, (char *) ctx->filename.data, ctx->filename.len));
			}
			break;
			
			case REQUEST_REMOTE_ADDR:
			*vp = STRING_TO_JSVAL(JS_NewStringCopyN(cx, (char *) r->connection->addr_text.data, r->connection->addr_text.len));
			break;
			
			case REQUEST_ARGS:
			*vp = STRING_TO_JSVAL(JS_NewStringCopyN(cx, (char *) r->args.data, r->args.len));
			break;
			
			case REQUEST_HEADER_ONLY:
			*vp = r->header_only ? JSVAL_TRUE : JSVAL_FALSE;
			break;
			
			case REQUEST_HEADERS_IN:
			E(headers = ngx_http_js__nginx_headers_in__wrap(cx, r), "Can`t ngx_http_js__nginx_headers_in__wrap()");
			*vp = OBJECT_TO_JSVAL(headers);
			break;
			
			case REQUEST_HEADERS_OUT:
			E(headers = ngx_http_js__nginx_headers_out__wrap(cx, r), "Can`t ngx_http_js__nginx_headers_out__wrap()");
			*vp = OBJECT_TO_JSVAL(headers);
			break;
			
			case REQUEST_BODY_FILENAME:
			if (r->request_body != NULL && r->request_body->temp_file != NULL)
				*vp = STRING_TO_JSVAL(JS_NewStringCopyN(cx, (char *) r->request_body->temp_file->file.name.data,
					r->request_body->temp_file->file.name.len));
			break;
			
			case REQUEST_BODY:
		    if (r->request_body != NULL && !r->request_body->temp_file && r->request_body->bufs != NULL)
			{
				size_t len = r->request_body->bufs->buf->last - r->request_body->bufs->buf->pos;
				// if (len >= 0)
					*vp = STRING_TO_JSVAL(JS_NewStringCopyN(cx, (char *) r->request_body->bufs->buf->pos, len));
			}
			break;
		}
	}
	return JS_TRUE;
}

JSPropertySpec ngx_http_js__nginx_request__props[] =
{
	{"uri",             REQUEST_URI,              JSPROP_READONLY|JSPROP_ENUMERATE,   NULL, NULL},
	{"method",          REQUEST_METHOD,           JSPROP_READONLY|JSPROP_ENUMERATE,   NULL, NULL},
	{"filename",        REQUEST_FILENAME,         JSPROP_READONLY|JSPROP_ENUMERATE,   NULL, NULL},
	{"remoteAddr",      REQUEST_REMOTE_ADDR,      JSPROP_READONLY|JSPROP_ENUMERATE,   NULL, NULL},
	{"headersIn",       REQUEST_HEADERS_IN,       JSPROP_READONLY|JSPROP_ENUMERATE,   NULL, NULL},
	{"headersOut",      REQUEST_HEADERS_OUT,      JSPROP_READONLY|JSPROP_ENUMERATE,   NULL, NULL},
	{"args",            REQUEST_ARGS,             JSPROP_READONLY|JSPROP_ENUMERATE,   NULL, NULL},
	{"headerOnly",      REQUEST_HEADER_ONLY,      JSPROP_READONLY|JSPROP_ENUMERATE,   NULL, NULL},
	{"bodyFilename",    REQUEST_BODY_FILENAME,    JSPROP_READONLY|JSPROP_ENUMERATE,   NULL, NULL},
	{"body",            REQUEST_BODY,             JSPROP_READONLY|JSPROP_ENUMERATE,   NULL, NULL},
	
	// TODO:
	// {"status",       MY_WIDTH,       JSPROP_ENUMERATE,  NULL, NULL},
	// {"requestBody",       MY_FUNNY,       JSPROP_ENUMERATE,  NULL, NULL},
	// {"allowRanges",       MY_ARRAY,       JSPROP_ENUMERATE,  NULL, NULL},
	{0, 0, 0, NULL, NULL}
};


JSFunctionSpec ngx_http_js__nginx_request__funcs[] = {
    {"sendHttpHeader",    method_sendHttpHeader,       0, 0, 0},
    {"printString",       method_printString,          1, 0, 0},
    {"flush",             method_flush,                0, 0, 0},
    {"sendString",        method_sendString,           1, 0, 0},
    {"subrequest",        method_subrequest,           2, 0, 0},
    {"cleanup",           method_cleanup,              0, 0, 0},
    {"sendSpecial",       method_sendSpecial,          1, 0, 0},
    {"discardBody",       method_discardBody,          0, 0, 0},
    {"hasBody",           method_hasBody,              1, 0, 0},
    {"sendfile",          method_sendfile,             1, 0, 0},
    {"setTimeout",        method_setTimeout,           2, 0, 0},
    {"nextBodyFilter",    method_nextBodyFilter,       1, 0, 0},
    {0, NULL, 0, 0, 0}
};

JSClass ngx_http_js__nginx_request__class =
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
	
	ngx_http_js__nginx_request__prototype = JS_InitClass(cx, nginxobj, NULL, &ngx_http_js__nginx_request__class,  request_constructor, 0,
		ngx_http_js__nginx_request__props, ngx_http_js__nginx_request__funcs,  NULL, NULL);
	E(ngx_http_js__nginx_request__prototype, "Can`t JS_InitClass(Nginx.Request)");
	
	return JS_TRUE;
}

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#include <js/jsapi.h>

#include <ngx_http_js_module.h>
#include <strings_util.h>
#include <classes/Request.h>

#include <nginx_js_macroses.h>


JSObject *ngx_http_js__nginx_headers_out__prototype;
JSClass ngx_http_js__nginx_headers_out__class;
static JSClass* private_class = &ngx_http_js__nginx_headers_out__class;

static ngx_str_t server_header_name           = ngx_string("Server");
static ngx_str_t date_header_name             = ngx_string("Date");
static ngx_str_t content_length_header_name   = ngx_string("Content-Length");

static ngx_table_elt_t *
search_headers_out(ngx_http_request_t *r, u_char *name, u_int len);

static void
delete_header(ngx_table_elt_t **hp);

static ngx_int_t
set_header(ngx_http_request_t *r, ngx_table_elt_t **hp, ngx_str_t *key, ngx_str_t *value);

static ngx_inline JSBool
set_header_from_jsval(JSContext *cx, ngx_http_request_t *r, ngx_table_elt_t **hp, ngx_str_t *key_ns, jsval *vp);

static ngx_inline JSBool
set_string_header_from_jsval(JSContext *cx, ngx_http_request_t *r, ngx_str_t *sp, jsval *vp);


JSObject *
ngx_http_js__nginx_headers_out__wrap(JSContext *cx, JSObject *request, ngx_http_request_t *r)
{
	JSObject                  *headers;
	ngx_http_js_ctx_t         *ctx;
	
	ctx = ngx_http_get_module_ctx(r, ngx_http_js_module);
	if (ctx == NULL)
	{
		return NULL;
	}
	
	if (ctx->js_headers_out != NULL)
	{
		return ctx->js_headers_out;
	}
	
	headers = JS_NewObject(cx, &ngx_http_js__nginx_headers_out__class, ngx_http_js__nginx_headers_out__prototype, NULL);
	if (headers == NULL)
	{
		JS_ReportOutOfMemory(cx);
		return NULL;
	}
	
	if (!JS_SetReservedSlot(cx, request, NGX_JS_REQUEST_SLOT__HEADERS_OUT, OBJECT_TO_JSVAL(headers)))
	{
		JS_ReportError(cx, "can't set slot NGX_JS_REQUEST_SLOT__HEADERS_OUT(%d)", NGX_JS_REQUEST_SLOT__HEADERS_OUT);
		return NULL;
	}
	
	JS_SetPrivate(cx, headers, r);
	
	ctx->js_headers_out = headers;
	
	return headers;
}


void
ngx_http_js__nginx_headers_out__cleanup(ngx_http_js_ctx_t *ctx, ngx_http_request_t *r, JSContext *cx)
{
	ngx_assert(ctx);
	
	if (!ctx->js_headers_out)
	{
		return;
	}
	
	JS_SetPrivate(cx, ctx->js_headers_out, NULL);
	ctx->js_headers_out = NULL;
}


static JSBool
constructor(JSContext *cx, JSObject *self, uintN argc, jsval *argv, jsval *rval)
{
	TRACE();
	return JS_TRUE;
}


// enum propid { HEADER_LENGTH };


static JSBool
getProperty(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	u_char                     *key;
	ngx_table_elt_t            *header;
	JSString                   *key_jsstr;
	
	TRACE();
	GET_PRIVATE(r);
	
	key_jsstr = JS_ValueToString(cx, id);
	if (key_jsstr == NULL)
	{
		return JS_FALSE;
	}
	
	key = (u_char *) JS_GetStringBytes(key_jsstr);
	if (key == NULL)
	{
		return JS_FALSE;
	}
	
	header = search_headers_out(r, key, 0);
	if (header == NULL || header->hash == 0)
	{
		*vp = JSVAL_VOID;
		return JS_TRUE;
	}
	
	NGX_STRING_to_JS_STRING_to_JSVAL(cx, header->value, *vp);
	
	return JS_TRUE;
}


static JSBool
setProperty(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	u_char                     *key;
	ngx_table_elt_t            *header;
	JSString                   *key_jss, *value_jss;
	
	TRACE();
	GET_PRIVATE(r);
	
	key_jss = JS_ValueToString(cx, id);
	if (key_jss == NULL)
	{
		return JS_FALSE;
	}
	
	key = (u_char *) JS_GetStringBytes(key_jss);
	if (key == NULL)
	{
		return JS_FALSE;
	}
	
	header = search_headers_out(r, key, 0);
	if (header == NULL)
	{
		header = ngx_list_push(&r->headers_out.headers);
		if (header == NULL)
		{
			JS_ReportOutOfMemory(cx);
			return JS_FALSE;
		}
		
		if (!js_str2ngx_str(cx, key_jss, r->pool, &header->key))
		{
			return JS_FALSE;
		}
	}
	
	if(JSVAL_IS_VOID(*vp))
	{
		header->hash = 0;
		return JS_TRUE;
	}
	
	value_jss = JS_ValueToString(cx, *vp);
	if (value_jss == NULL)
	{
		return JS_FALSE;
	}
	
	if (!js_str2ngx_str(cx, value_jss, r->pool, &header->value))
	{
		return JS_FALSE;
	}
	
	header->hash = header->value.len == 0 ? 0 : 1;
	
	return JS_TRUE;
}

static JSBool
delProperty(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	TRACE();
	return JS_TRUE;
}


static JSBool
getter_server(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	
	TRACE();
	GET_PRIVATE(r);
	
	if (r->headers_out.server == NULL || r->headers_out.server->hash == 0)
	{
		*vp = JSVAL_VOID;
		return JS_TRUE;
	}
	
	NGX_STRING_to_JS_STRING_to_JSVAL(cx, r->headers_out.server->value, *vp);
	
	return JS_TRUE;
}

static JSBool
setter_server(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	
	TRACE();
	GET_PRIVATE(r);
	
	if (!set_header_from_jsval(cx, r, &r->headers_out.server, &server_header_name, vp))
	{
		return JS_FALSE;
	}
	
	return JS_TRUE;
}


static JSBool
getter_date(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	
	TRACE();
	GET_PRIVATE(r);
	
	if (r->headers_out.date == NULL || r->headers_out.date->hash == 0)
	{
		*vp = JSVAL_VOID;
		return JS_TRUE;
	}
	
	NGX_STRING_to_JS_STRING_to_JSVAL(cx, r->headers_out.date->value, *vp);
	
	return JS_TRUE;
}

static JSBool
setter_date(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	
	TRACE();
	GET_PRIVATE(r);
	
	if (!set_header_from_jsval(cx, r, &r->headers_out.date, &date_header_name, vp))
	{
		return JS_FALSE;
	}
	
	if (r->headers_out.date == NULL || r->headers_out.date->value.len == 0)
	{
		r->headers_out.date_time = -1;
		return JS_TRUE;
	}
	
	r->headers_out.date_time = ngx_http_parse_time(r->headers_out.date->value.data, r->headers_out.date->value.len);
	
	return JS_TRUE;
}


static JSBool
getter_dateTime(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	
	TRACE();
	GET_PRIVATE(r);
	
	if (!JS_NewNumberValue(cx, r->headers_out.date_time, vp))
	{
		return JS_FALSE;
	}
	
	return JS_TRUE;
}


static JSBool
getter_contentLength(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	
	TRACE();
	GET_PRIVATE(r);
	
	if (r->headers_out.content_length == NULL || r->headers_out.content_length->hash == 0)
	{
		*vp = JSVAL_VOID;
		return JS_TRUE;
	}
	
	NGX_STRING_to_JS_STRING_to_JSVAL(cx, r->headers_out.content_length->value, *vp);
	
	return JS_TRUE;
}

static JSBool
setter_contentLength(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	jsdouble                    val_n;
	
	TRACE();
	GET_PRIVATE(r);
	
	if (!set_header_from_jsval(cx, r, &r->headers_out.content_length, &content_length_header_name, vp))
	{
		return JS_FALSE;
	}
	
	if (r->headers_out.content_length == NULL || r->headers_out.content_length->value.len == 0)
	{
		r->headers_out.content_length_n = 0;
		return JS_TRUE;
	}
	
	if (!JS_ValueToNumber(cx, *vp, &val_n))
	{
		return JS_FALSE;
	}
	
	r->headers_out.content_length_n = val_n;//ngx_http_parse_time(r->headers_out.date->value.data, r->headers_out.date->value.len);
	
	return JS_TRUE;
}

static JSBool
getter_contentLengthN(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	
	TRACE();
	GET_PRIVATE(r);
	
	if (!JS_NewNumberValue(cx, r->headers_out.content_length_n, vp))
	{
		return JS_FALSE;
	}
	
	return JS_TRUE;
}


static JSBool
getter_contentEncoding(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	
	TRACE();
	GET_PRIVATE(r);
	
	if (r->headers_out.content_encoding == NULL || r->headers_out.content_encoding->hash == 0)
	{
		*vp = JSVAL_VOID;
		return JS_TRUE;
	}
	
	NGX_STRING_to_JS_STRING_to_JSVAL(cx, r->headers_out.content_encoding->value, *vp);
	
	return JS_TRUE;
}

static JSBool
setter_contentEncoding(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	static ngx_str_t            header_name = ngx_string("Content-Encoding");
	
	TRACE();
	GET_PRIVATE(r);
	
	if (!set_header_from_jsval(cx, r, &r->headers_out.content_encoding, &header_name, vp))
	{
		return JS_FALSE;
	}
	
	return JS_TRUE;
}


static JSBool
getter_location(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	
	TRACE();
	GET_PRIVATE(r);
	
	if (r->headers_out.location == NULL || r->headers_out.location->hash == 0)
	{
		*vp = JSVAL_VOID;
		return JS_TRUE;
	}
	
	NGX_STRING_to_JS_STRING_to_JSVAL(cx, r->headers_out.location->value, *vp);
	
	return JS_TRUE;
}

static JSBool
setter_location(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	static ngx_str_t            header_name = ngx_string("Location");
	
	TRACE();
	GET_PRIVATE(r);
	
	if (!set_header_from_jsval(cx, r, &r->headers_out.location, &header_name, vp))
	{
		return JS_FALSE;
	}
	
	return JS_TRUE;
}

static JSBool
getter_refresh(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	
	TRACE();
	GET_PRIVATE(r);
	
	if (r->headers_out.refresh == NULL || r->headers_out.refresh->hash == 0)
	{
		*vp = JSVAL_VOID;
		return JS_TRUE;
	}
	
	NGX_STRING_to_JS_STRING_to_JSVAL(cx, r->headers_out.refresh->value, *vp);
	
	return JS_TRUE;
}

static JSBool
setter_refresh(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	static ngx_str_t            header_name = ngx_string("Refresh");
	
	TRACE();
	GET_PRIVATE(r);
	
	if (!set_header_from_jsval(cx, r, &r->headers_out.refresh, &header_name, vp))
	{
		return JS_FALSE;
	}
	
	return JS_TRUE;
}


static JSBool
getter_lastModified(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	
	TRACE();
	GET_PRIVATE(r);
	
	if (r->headers_out.last_modified == NULL || r->headers_out.last_modified->hash == 0)
	{
		*vp = JSVAL_VOID;
		return JS_TRUE;
	}
	
	NGX_STRING_to_JS_STRING_to_JSVAL(cx, r->headers_out.last_modified->value, *vp);
	
	return JS_TRUE;
}

static JSBool
setter_lastModified(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	static ngx_str_t            header_name = ngx_string("Location");
	
	TRACE();
	GET_PRIVATE(r);
	
	if (!set_header_from_jsval(cx, r, &r->headers_out.last_modified, &header_name, vp))
	{
		return JS_FALSE;
	}
	
	if (r->headers_out.last_modified == NULL || r->headers_out.last_modified->value.len == 0)
	{
		r->headers_out.last_modified_time = -1;
		return JS_TRUE;
	}
	
	r->headers_out.last_modified_time = ngx_http_parse_time(r->headers_out.last_modified->value.data, r->headers_out.last_modified->value.len);
	
	return JS_TRUE;
}


static JSBool
getter_lastModifiedTime(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	
	TRACE();
	GET_PRIVATE(r);
	
	if (!JS_NewNumberValue(cx, r->headers_out.last_modified_time, vp))
	{
		return JS_FALSE;
	}
	
	return JS_TRUE;
}


static JSBool
getter_contentRange(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	
	TRACE();
	GET_PRIVATE(r);
	
	if (r->headers_out.content_range == NULL || r->headers_out.content_range->hash == 0)
	{
		*vp = JSVAL_VOID;
		return JS_TRUE;
	}
	
	NGX_STRING_to_JS_STRING_to_JSVAL(cx, r->headers_out.content_range->value, *vp);
	
	return JS_TRUE;
}

static JSBool
setter_contentRange(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	static ngx_str_t            header_name = ngx_string("Content-Range");
	
	TRACE();
	GET_PRIVATE(r);
	
	if (!set_header_from_jsval(cx, r, &r->headers_out.content_range, &header_name, vp))
	{
		return JS_FALSE;
	}
	
	return JS_TRUE;
}


static JSBool
getter_acceptRanges(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	
	TRACE();
	GET_PRIVATE(r);
	
	if (r->headers_out.accept_ranges == NULL || r->headers_out.accept_ranges->hash == 0)
	{
		*vp = JSVAL_VOID;
		return JS_TRUE;
	}
	
	NGX_STRING_to_JS_STRING_to_JSVAL(cx, r->headers_out.accept_ranges->value, *vp);
	
	return JS_TRUE;
}

static JSBool
setter_acceptRanges(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	static ngx_str_t            header_name = ngx_string("Accept-Ranges");
	
	TRACE();
	GET_PRIVATE(r);
	
	if (!set_header_from_jsval(cx, r, &r->headers_out.accept_ranges, &header_name, vp))
	{
		return JS_FALSE;
	}
	
	return JS_TRUE;
}


static JSBool
getter_wwwAuthenticate(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	
	TRACE();
	GET_PRIVATE(r);
	
	if (r->headers_out.www_authenticate == NULL || r->headers_out.www_authenticate->hash == 0)
	{
		*vp = JSVAL_VOID;
		return JS_TRUE;
	}
	
	NGX_STRING_to_JS_STRING_to_JSVAL(cx, r->headers_out.www_authenticate->value, *vp);
	
	return JS_TRUE;
}

static JSBool
setter_wwwAuthenticate(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	static ngx_str_t            header_name = ngx_string("WWW-Authenticate");
	
	TRACE();
	GET_PRIVATE(r);
	
	if (!set_header_from_jsval(cx, r, &r->headers_out.www_authenticate, &header_name, vp))
	{
		return JS_FALSE;
	}
	
	return JS_TRUE;
}


static JSBool
getter_expires(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	
	TRACE();
	GET_PRIVATE(r);
	
	if (r->headers_out.expires == NULL || r->headers_out.expires->hash == 0)
	{
		*vp = JSVAL_VOID;
		return JS_TRUE;
	}
	
	NGX_STRING_to_JS_STRING_to_JSVAL(cx, r->headers_out.expires->value, *vp);
	
	return JS_TRUE;
}

static JSBool
setter_expires(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	static ngx_str_t            header_name = ngx_string("Expires");
	
	TRACE();
	GET_PRIVATE(r);
	
	if (!set_header_from_jsval(cx, r, &r->headers_out.expires, &header_name, vp))
	{
		return JS_FALSE;
	}
	
	return JS_TRUE;
}


static JSBool
getter_etag(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	
	TRACE();
	GET_PRIVATE(r);
	
	if (r->headers_out.etag == NULL || r->headers_out.etag->hash == 0)
	{
		*vp = JSVAL_VOID;
		return JS_TRUE;
	}
	
	NGX_STRING_to_JS_STRING_to_JSVAL(cx, r->headers_out.etag->value, *vp);
	
	return JS_TRUE;
}

static JSBool
setter_etag(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	static ngx_str_t            header_name = ngx_string("ETag");
	
	TRACE();
	GET_PRIVATE(r);
	
	if (!set_header_from_jsval(cx, r, &r->headers_out.etag, &header_name, vp))
	{
		return JS_FALSE;
	}
	
	return JS_TRUE;
}


static JSBool
getter_contentType(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	
	TRACE();
	GET_PRIVATE(r);
	
	if (r->headers_out.content_type.data == NULL || r->headers_out.content_type.len == 0)
	{
		*vp = JSVAL_VOID;
		return JS_TRUE;
	}
	
	NGX_STRING_to_JS_STRING_to_JSVAL(cx, r->headers_out.content_type, *vp);
	
	return JS_TRUE;
}

static JSBool
setter_contentType(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	
	TRACE();
	GET_PRIVATE(r);
	
	if (!set_string_header_from_jsval(cx, r, &r->headers_out.content_type, vp))
	{
		return JS_FALSE;
	}
	
	r->headers_out.content_type_len = r->headers_out.content_type.len;
	
	// reset the lowercased version
	r->headers_out.content_type_lowcase = NULL;
	// reset the hash of the lowercased version
	r->headers_out.content_type_hash = 0;
	
	return JS_TRUE;
}

static JSBool
getter_contentTypeLen(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	
	TRACE();
	GET_PRIVATE(r);
	
	if (!JS_NewNumberValue(cx, r->headers_out.content_type_len, vp))
	{
		return JS_FALSE;
	}
	
	return JS_TRUE;
}

static JSBool
getter_contentTypeLowcase(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	
	TRACE();
	GET_PRIVATE(r);
	
	if (r->headers_out.content_type_lowcase == NULL)
	{
		*vp = JSVAL_VOID;
		return JS_TRUE;
	}
	
	DATA_LEN_to_JS_STRING_to_JSVAL(cx, r->headers_out.content_type_lowcase, ngx_strlen(r->headers_out.content_type_lowcase), *vp);
	
	return JS_TRUE;
}


static JSBool
getter_cacheControl(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	ngx_table_elt_t           **ph;
	ngx_uint_t                  i, n;
	ssize_t                     size;
	u_char                     *p, *data;
	
	TRACE();
	GET_PRIVATE(r);
	
	n = r->headers_out.cache_control.nelts;
	
	if (n == 0)
	{
		*vp = JSVAL_VOID;
		return JS_TRUE;
	}
	
	ph = r->headers_out.cache_control.elts;
	
	if (ph == NULL)
	{
		*vp = JSVAL_VOID;
		return JS_TRUE;
	}
	
	if (n == 1)
	{
		NGX_STRING_to_JS_STRING_to_JSVAL(cx, (*ph)->value, *vp);
		return JS_TRUE;
	}
	
	size = - (ssize_t) (sizeof("; ") - 1);
	
	for (i = 0; i < n; i++)
	{
		if (ph[i]->hash == 0)
		{
			continue;
		}
		size += ph[i]->value.len + sizeof("; ") - 1;
	}
	
	// all headers was disabled
	if (size == - (ssize_t) (sizeof("; ") - 1))
	{
		*vp = JSVAL_VOID;
		return JS_TRUE;
	}
	
	data = ngx_pnalloc(r->pool, size);
	if (data == NULL)
	{
		JS_ReportOutOfMemory(cx);
		return JS_FALSE;
	}
	
	p = data;
	
	for (i = 0; /* void */ ; i++)
	{
		if (ph[i]->hash == 0)
		{
			continue;
		}
		
		p = ngx_copy(p, ph[i]->value.data, ph[i]->value.len);
		
		if (i == n - 1)
		{
			break;
		}
		
		*p++ = ';'; *p++ = ' ';
	}
	
	DATA_LEN_to_JS_STRING_to_JSVAL(cx, data, size, *vp);
	
	return JS_TRUE;
}

static JSBool
setter_cacheControl(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	ngx_table_elt_t            *cc, **ccp;
	
	TRACE();
	GET_PRIVATE(r);
	
	// For now the undefined value leads to header deletion. The “delete” keyword
	// may be implemented only via Class.delProperty on the tinyId basis.
	if (JSVAL_IS_VOID(*vp))
	{
		ngx_uint_t i;
		
		ccp = r->headers_out.cache_control.elts;
		
		if (ccp == NULL)
		{
			// empty already
			return JS_TRUE;
		}
		
		// disable all headers
		for (i = 0; i < r->headers_out.cache_control.nelts; i++)
		{
			ccp[i]->hash = 0;
		}
		
		r->headers_out.cache_control.elts = NULL;
		
		return JS_TRUE;
	}
	
	
	ccp = r->headers_out.cache_control.elts;
	
	if (ccp == NULL)
	{
		if (ngx_array_init(&r->headers_out.cache_control, r->pool, 1, sizeof(ngx_table_elt_t *)) != NGX_OK)
		{
			JS_ReportOutOfMemory(cx);
			return JS_FALSE;
		}
		
		ccp = ngx_array_push(&r->headers_out.cache_control);
		if (ccp == NULL)
		{
			JS_ReportOutOfMemory(cx);
			return JS_FALSE;
		}
		
		cc = ngx_list_push(&r->headers_out.headers);
		if (cc == NULL)
		{
			JS_ReportOutOfMemory(cx);
			return JS_FALSE;
		}
		
		cc->hash = 1;
		cc->key.len = sizeof("Cache-Control") - 1;
		cc->key.data = (u_char *) "Cache-Control";
		
		*ccp = cc;
	}
	else
	{
		ngx_uint_t i;
		
		for (i = 1; i < r->headers_out.cache_control.nelts; i++)
		{
			ccp[i]->hash = 0;
		}
		
		cc = ccp[0];
	}
	
	if (!set_string_header_from_jsval(cx, r, &cc->value, vp))
	{
		return JS_FALSE;
	}
	
	if (cc->value.len == 0)
	{
		// dirty copy-paste
		
		ngx_uint_t i;
		
		ccp = r->headers_out.cache_control.elts;
		
		if (ccp == NULL)
		{
			// empty already
			return JS_TRUE;
		}
		
		// disable all headers
		for (i = 0; i < r->headers_out.cache_control.nelts; i++)
		{
			ccp[i]->hash = 0;
		}
		
		r->headers_out.cache_control.elts = NULL;
		
		return JS_TRUE;
	}
	
	return JS_TRUE;
}


JSPropertySpec ngx_http_js__nginx_headers_out__props[] =
{
	{"Server",                       0,  JSPROP_ENUMERATE,                       getter_server,         setter_server},
	{"Date",                         0,  JSPROP_ENUMERATE,                       getter_date,           setter_date},
	{"$dateTime",                    0,  JSPROP_ENUMERATE | JSPROP_READONLY,     getter_dateTime,       NULL},
	{"Content-Length",               0,  JSPROP_ENUMERATE,                       getter_contentLength,  setter_contentLength},
	{"$contentLength",               0,  JSPROP_ENUMERATE | JSPROP_READONLY,     getter_contentLengthN, NULL},
	{"Content-Encoding",             0,  JSPROP_ENUMERATE,                       getter_contentEncoding,setter_contentEncoding},
	{"Location",                     0,  JSPROP_ENUMERATE,                       getter_location,       setter_location},
	{"Refresh",                      0,  JSPROP_ENUMERATE,                       getter_refresh,        setter_refresh},
	{"Last-Modified",                0,  JSPROP_ENUMERATE,                       getter_lastModified,   setter_lastModified},
	{"$lastModified",                0,  JSPROP_ENUMERATE | JSPROP_READONLY,     getter_lastModifiedTime, NULL},
	{"Content-Range",                0,  JSPROP_ENUMERATE,                       getter_contentRange,   setter_contentRange},
	{"Accept-Ranges",                0,  JSPROP_ENUMERATE,                       getter_acceptRanges,   setter_acceptRanges},
	{"WWW-Authenticate",             0,  JSPROP_ENUMERATE,                       getter_wwwAuthenticate,setter_wwwAuthenticate},
	{"Expires",                      0,  JSPROP_ENUMERATE,                       getter_expires,        setter_expires},
	{"ETag",                         0,  JSPROP_ENUMERATE,                       getter_etag,           setter_etag},
	{"Content-Type",                 0,  JSPROP_ENUMERATE,                       getter_contentType,    setter_contentType},
	{"$contentTypeLen",              0,  JSPROP_ENUMERATE,                       getter_contentTypeLen, NULL},
	{"$contentTypeLowcase",          0,  JSPROP_ENUMERATE,                       getter_contentTypeLowcase, NULL},
	{"Cache-Control",                0,  JSPROP_ENUMERATE,                       getter_cacheControl,   setter_cacheControl},
	{0, 0, 0, NULL, NULL}
};


JSFunctionSpec ngx_http_js__nginx_headers_out__funcs[] =
{
	{0, NULL, 0, 0, 0}
};

JSClass ngx_http_js__nginx_headers_out__class =
{
	"HeadersOut",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, delProperty, getProperty, setProperty,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

JSBool
ngx_http_js__nginx_headers_out__init(JSContext *cx, JSObject *global)
{
	JSObject    *nginxobj;
	jsval        vp;
	
	E(JS_GetProperty(cx, global, "Nginx", &vp), "global.Nginx is undefined or is not a function");
	nginxobj = JSVAL_TO_OBJECT(vp);
	
	ngx_http_js__nginx_headers_out__prototype = JS_InitClass(cx, nginxobj, NULL, &ngx_http_js__nginx_headers_out__class,  constructor, 0,
		ngx_http_js__nginx_headers_out__props, ngx_http_js__nginx_headers_out__funcs,  NULL, NULL);
	E(ngx_http_js__nginx_headers_out__prototype, "Can`t JS_InitClass(Nginx.HeadersOut)");
	
	return JS_TRUE;
}


static ngx_table_elt_t *
search_headers_out(ngx_http_request_t *r, u_char *name, u_int len)
{
	ngx_list_part_t            *part;
	ngx_table_elt_t            *h;
	ngx_uint_t                  i;
	
	TRACE();
	ngx_assert(r);
	ngx_assert(name);
	
	if (len == 0)
	{
		len = strlen((char *) name);
		if (len == 0)
		{
			return NULL;
		}
	}
	
	// look in all headers
	
	part = &r->headers_out.headers.part;
	h = part->elts;
	
	for (i = 0; /* void */ ; i++)
	{
		if (i >= part->nelts)
		{
			if (part->next == NULL)
			{
				break;
			}
			
			part = part->next;
			h = part->elts;
			i = 0;
		}
		
		// LOG("%s", h[i].key.data);
		if (len != h[i].key.len || ngx_strcasecmp(name, h[i].key.data) != 0)
		{
			continue;
		}
		
		return &h[i];
	}
	
	return NULL;
}

static void
delete_header(ngx_table_elt_t **hp)
{
	ngx_table_elt_t   *h;
	
	h = *hp;
	if (h == NULL)
	{
		// header is not exists
		return;
	}
	
	// This means a “deleted” header for ngx_http_header_filter()
	// at ngx_http_header_filter_module.c:428. So we do not have to delete
	// the header from r->headers_out.headers.
	h->hash = 0;
	
	// Mark the headers as unexisted for ngx_http_header_filter()
	// at ngx_http_header_filter_module.c:453.
	*hp = NULL;
}

static ngx_int_t
set_header(ngx_http_request_t *r, ngx_table_elt_t **hp, ngx_str_t *key, ngx_str_t *value)
{
	ngx_table_elt_t   *h;
	
	h = *hp;
	// if the header is not exists yet
	if (h == NULL)
	{
		// tryin to allocate header element
		h = ngx_list_push(&r->headers_out.headers);
		if (h == NULL)
		{
			return NGX_ERROR;
		}
		
		*hp = h;
	}
	
	h->hash = value->len == 0 ? 0 : 1;
	h->key = *key;
	h->value = *value;
	
	return NGX_OK;
}

static ngx_inline JSBool
set_header_from_jsval(JSContext *cx, ngx_http_request_t *r, ngx_table_elt_t **hp, ngx_str_t *key_ns, jsval *vp)
{
	ngx_str_t                   value_ns;
	JSString                   *value;
	
	// For now the undefined value leads to header deletion. The “delete” keyword
	// may be implemented only via Class.delProperty on the tinyId basis.
	if (JSVAL_IS_VOID(*vp))
	{
		delete_header(hp);
		return JS_TRUE;
	}
	
	value = JS_ValueToString(cx, *vp);
	if (value == NULL)
	{
		return JS_FALSE;
	}
	
	if (!js_str2ngx_str(cx, value, r->pool, &value_ns))
	{
		return JS_FALSE;
	}
	
	if (set_header(r, hp, key_ns, &value_ns) != NGX_OK)
	{
		JS_ReportOutOfMemory(cx);
		return JS_FALSE;
	}
	
	return JS_TRUE;
}

static ngx_inline JSBool
set_string_header_from_jsval(JSContext *cx, ngx_http_request_t *r, ngx_str_t *sp, jsval *vp)
{
	JSString                   *value;
	
	// For now the undefined value leads to header deletion. The “delete” keyword
	// may be implemented only via Class.delProperty on the tinyId basis.
	if (JSVAL_IS_VOID(*vp))
	{
		sp->len = 0;
		sp->data = NULL;
		return JS_TRUE;
	}
	
	value = JS_ValueToString(cx, *vp);
	if (value == NULL)
	{
		return JS_FALSE;
	}
	
	if (!js_str2ngx_str(cx, value, r->pool, sp))
	{
		return JS_FALSE;
	}
	
	return JS_TRUE;
}

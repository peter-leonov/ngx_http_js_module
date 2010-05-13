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
search_headers_out(ngx_http_request_t *r, char *name, u_int len);

static void
delete_header(ngx_table_elt_t **hp);

static ngx_int_t
set_header(ngx_http_request_t *r, ngx_table_elt_t **hp, ngx_str_t *key, ngx_str_t *value);

static ngx_inline JSBool
set_header_from_jsval(JSContext *cx, ngx_http_request_t *r, ngx_table_elt_t **hp, ngx_str_t *key_ns, jsval *vp);


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
	char                       *name;
	ngx_table_elt_t            *header;
	
	TRACE();
	GET_PRIVATE(r);
	
	if (JSVAL_IS_STRING(id) && (name = JS_GetStringBytes(JSVAL_TO_STRING(id))) != NULL)
	{
		header = search_headers_out(r, name, 0);
		if (header != NULL)
		{
			NGX_STRING_to_JS_STRING_to_JSVAL(cx, header->value, *vp);
		}
	}
	
	return JS_TRUE;
}


static JSBool
setProperty(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	ngx_table_elt_t            *header;
	char                       *key;
	size_t                      key_len;
	JSString                   *key_jsstr, *value_jsstr;
	
	TRACE();
	GET_PRIVATE(r);
	
	if (JSVAL_IS_STRING(id))
	{
		key_jsstr = JSVAL_TO_STRING(id);
		E(key = js_str2c_str(cx, key_jsstr, r->pool, &key_len), "Can`t js_str2c_str(key_jsstr)");
		E(value_jsstr = JS_ValueToString(cx, *vp), "Can`t JS_ValueToString()");
		
		
		header = search_headers_out(r, key, key_len);
		if (header != NULL)
		{
			header->key.data = (u_char*)key;
			header->key.len = key_len;
			E(js_str2ngx_str(cx, value_jsstr, r->pool, &header->value), "Can`t js_str2ngx_str(value_jsstr)");
			
			return JS_TRUE;
		}
		
		
		header = ngx_list_push(&r->headers_out.headers);
		if (header == NULL)
		{
			JS_ReportOutOfMemory(cx);
			return JS_FALSE;
		}
		
		header->hash = 1;
		
		header->key.data = (u_char*)key;
		header->key.len = key_len;
		E(js_str2ngx_str(cx, value_jsstr, r->pool, &header->value), "Can`t js_str2ngx_str(value_jsstr)");
		
		if (NCASE_COMPARE(header->key, "Content-Length"))
		{
			E(JSVAL_IS_INT(*vp), "the Content-Length value must be an Integer");
			r->headers_out.content_length_n = (off_t) JSVAL_TO_INT(*vp);
			r->headers_out.content_length = header;
			
			return JS_TRUE;
		}
	}
	
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
delProperty(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	TRACE();
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
search_headers_out(ngx_http_request_t *r, char *name, u_int len)
{
	ngx_list_part_t            *part;
	ngx_table_elt_t            *h;
	ngx_uint_t                  i;
	
	TRACE();
	ngx_assert(r);
	ngx_assert(name);
	
	if (len == 0)
	{
		len = strlen(name);
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
		if (len != h[i].key.len || ngx_strcasecmp((u_char *) name, h[i].key.data) != 0)
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

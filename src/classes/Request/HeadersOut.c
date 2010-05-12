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

static ngx_table_elt_t *
search_headers_out(ngx_http_request_t *r, char *name, u_int len);

static void
delete_header(ngx_table_elt_t **hp);

static ngx_int_t
set_header(ngx_http_request_t *r, ngx_table_elt_t **hp, ngx_str_t *key, ngx_str_t *value);


JSObject *
ngx_http_js__nginx_headers_out__wrap(JSContext *cx, JSObject *request, ngx_http_request_t *r)
{
	JSObject                  *headers;
	ngx_http_js_ctx_t         *ctx;
	
	if (!(ctx = ngx_http_get_module_ctx(r, ngx_http_js_module)))
		return NULL;
	
	if (ctx->js_headers_out)
		return ctx->js_headers_out;
	
	headers = JS_NewObject(cx, &ngx_http_js__nginx_headers_out__class, ngx_http_js__nginx_headers_out__prototype, NULL);
	if (!headers)
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
		return;
	
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
		// if (!strcmp(member_name, "constructor"))
		// LOG("getProperty: %s", name);
		
		header = search_headers_out(r, name, 0);
		
		if (header != NULL)
		{
			NGX_STRING_to_JS_STRING_to_JSVAL(cx, header->value, *vp);
		}
		// else
		// 	LOG("getProperty: %s was not found", name);
	}
	
	// *vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, "not set"));
	
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
	
	// E(JSVAL_IS_STRING(id), "Nginx.Request#[]= takes a key:String and a value of a key relational type");
	if (JSVAL_IS_STRING(id))
	{
		key_jsstr = JSVAL_TO_STRING(id);
		E(key = js_str2c_str(cx, key_jsstr, r->pool, &key_len), "Can`t js_str2c_str(key_jsstr)");
		E(value_jsstr = JS_ValueToString(cx, *vp), "Can`t JS_ValueToString()");
		
		// LOG("setProperty: %s (%u)", key, (int)key_len);
		
		header = search_headers_out(r, key, key_len);
		if (header)
		{
			header->key.data = (u_char*)key;
			header->key.len = key_len;
			E(js_str2ngx_str(cx, value_jsstr, r->pool, &header->value), "Can`t js_str2ngx_str(value_jsstr)");
			// LOG("by hash");
			return JS_TRUE;
		}
		
		
		header = ngx_list_push(&r->headers_out.headers);
		if (header)
		{
			header->hash = 1;
			
			header->key.data = (u_char*)key;
			header->key.len = key_len;
			E(js_str2ngx_str(cx, value_jsstr, r->pool, &header->value), "Can`t js_str2ngx_str(value_jsstr)");
			
			if (NCASE_COMPARE(header->key, "Content-Length"))
			{
				E(JSVAL_IS_INT(*vp), "the Content-Length value must be an Integer");
				r->headers_out.content_length_n = (off_t) JSVAL_TO_INT(*vp);
				r->headers_out.content_length = header;
				// LOG("by list");
				return JS_TRUE;
			}
		}
		else
			THROW("Can`t ngx_list_push()");
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
	ngx_str_t                   value_ns, key_ns = ngx_string("Server");
	JSString                   *value;
	
	TRACE();
	GET_PRIVATE(r);
	
	// For now the undefined value leads to header deletion. The “delete” keyword
	// may be implemented only via Class.delProperty on the tinyId basis.
	if (JSVAL_IS_VOID(*vp))
	{
		delete_header(&r->headers_out.server);
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
	
	if (set_header(r, &r->headers_out.server, &key_ns, &value_ns) != NGX_OK)
	{
		JS_ReportOutOfMemory(cx);
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
	{"Server",                       0,  JSPROP_ENUMERATE,   getter_server, setter_server},
	{0, 0, 0, NULL, NULL}
};


JSFunctionSpec ngx_http_js__nginx_headers_out__funcs[] = {
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
			return NULL;
	}
	
	// look in all headers
	
	part = &r->headers_out.headers.part;
	h = part->elts;
	
	for (i = 0; /* void */ ; i++)
	{
		if (i >= part->nelts)
		{
			if (part->next == NULL)
				break;
			
			part = part->next;
			h = part->elts;
			i = 0;
		}
		
		// LOG("%s", h[i].key.data);
		if (len != h[i].key.len || ngx_strcasecmp((u_char *) name, h[i].key.data) != 0)
			continue;
		
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
	
	h->hash = 1;
	h->key = *key;
	h->value = *value;
	
	return NGX_OK;
}

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#include <ngx_http_jsapi.h>

#include <ngx_http_js_module.h>
#include <nginx_js_glue.h>
#include <strings_util.h>
#include <classes/Request.h>
#include <classes/Request/HeadersIn/Cookies.h>

#include <nginx_js_macroses.h>


JSObject *ngx_http_js__nginx_headers_in__prototype;
JSClass ngx_http_js__nginx_headers_in__class;
static JSClass* private_class = &ngx_http_js__nginx_headers_in__class;

static ngx_inline ngx_int_t
hash_header_name(u_char *lowercased, u_char *name, size_t len);

static ngx_inline ngx_http_header_t *
search_hashed_headers_in(ngx_http_request_t *r, ngx_int_t hash, u_char *lowercased, size_t len);

static ngx_table_elt_t *
search_headers_in(ngx_http_request_t *r, u_char *name, size_t len);


JSObject *
ngx_http_js__nginx_headers_in__wrap(JSContext *cx, JSObject *request, ngx_http_request_t *r)
{
	JSObject                  *headers;
	ngx_http_js_ctx_t         *ctx;
	
	ctx = ngx_http_get_module_ctx(r, ngx_http_js_module);
	if (ctx == NULL)
	{
		return NULL;
	}
	
	if (ctx->js_headers_in != NULL)
	{
		return ctx->js_headers_in;
	}
	
	headers = JS_NewObject(cx, &ngx_http_js__nginx_headers_in__class, ngx_http_js__nginx_headers_in__prototype, NULL);
	if (headers == NULL)
	{
		JS_ReportOutOfMemory(cx);
		return NULL;
	}
	
	if (!JS_SetReservedSlot(cx, request, NGX_JS_REQUEST_SLOT__HEADERS_IN, OBJECT_TO_JSVAL(headers)))
	{
		JS_ReportError(cx, "can't set slot NGX_JS_REQUEST_SLOT__HEADERS_IN(%d)", NGX_JS_REQUEST_SLOT__HEADERS_IN);
		return NULL;
	}
	
	JS_SetPrivate(cx, headers, r);
	
	ctx->js_headers_in = headers;
	
	return headers;
}


void
ngx_http_js__nginx_headers_in__cleanup(ngx_http_js_ctx_t *ctx, ngx_http_request_t *r, JSContext *cx)
{
	ngx_assert(ctx);
	
	if (ctx->js_headers_in == NULL)
	{
		return;
	}
	
	JS_SetPrivate(cx, ctx->js_headers_in, NULL);
	ctx->js_headers_in = NULL;
}


static JSBool
constructor(JSContext *cx, uintN argc, jsval *vp)
{
	TRACE();
	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;
}

#define NGX_HEADER_to_JSVAL(header, v) \
if (header == NULL) \
{ \
	v = JSVAL_NULL; \
} \
else \
{ \
	NGX_STRING_to_JS_STRING_to_JSVAL(cx, header->value, v); \
}


static JSBool
getProperty(JSContext *cx, JSObject *self, jsid rawid, jsval *vp)
{
	ngx_http_request_t    *r;
	jsval                  id;
	
	TRACE();
	GET_PRIVATE(r);
	
	if (JS_IdToValue(cx, rawid, &id))
	{
		return JS_FALSE;
	}
	
	if (JSVAL_IS_INT(id))
	{
		switch (JSVAL_TO_INT(id))
		{
			case 1:
			{
				JSObject   *js_cookies;
				js_cookies = ngx_http_js__nginx_cookies__wrap(cx, r);
				if (js_cookies == NULL)
				{
					// just forward the exception
					return JS_FALSE;
				}
				*vp = OBJECT_TO_JSVAL(js_cookies);
			}
			break;
			
			case 100:
			NGX_HEADER_to_JSVAL(r->headers_in.content_length, *vp);
			break;
			
			case 101:
			{
				if (!JS_NewNumberValue(cx, r->headers_in.content_length_n, vp))
				{
					return JS_FALSE;
				}
			}
			break;
			
			case 102:
			NGX_HEADER_to_JSVAL(r->headers_in.range, *vp);
			break;
			
		}
	}
	else if (JSVAL_IS_STRING(id))
	{
		ngx_table_elt_t            *header;
		ngx_http_header_t          *hh;
		ngx_int_t                   hash;
		u_char                     *key, *lowercased;
		u_int                       len;
		
		
		key = (u_char *) JS_GetStringBytes(JSVAL_TO_STRING(id));
		if (key == NULL)
		{
			return JS_FALSE;
		}
		
		len = ngx_strlen(key);
		
		if (len == 0)
		{
			// just return an undefined value
			return JS_TRUE;
		}
		
		lowercased = ngx_palloc(r->pool, len);
		if (lowercased == NULL)
		{
			JS_ReportOutOfMemory(cx);
			return JS_FALSE;
		}
		
		hash = hash_header_name(lowercased, key, len);
		
		header = NULL;
		
		hh = search_hashed_headers_in(r, hash, lowercased, len);
		if (hh != NULL)
		{
			// and this means its value was already cached in some field
			// of the r->headers_in stuct (hh->offset tells which)
			if (hh->offset)
			{
				// we got the element of the r->headers_in.headers
				// without brute forcing through all headers names
				header = *((ngx_table_elt_t **) ((char *) &r->headers_in + hh->offset));
			}
		}
		
		if (header == NULL)
		{
			header = search_headers_in(r, key, len);
		}
		
		if (header != NULL)
		{
			NGX_STRING_to_JS_STRING_to_JSVAL(cx, header->value, *vp);
		}
		
		// here we assume that all those headers like Content-Length
		// are represented identically in both the text form:
		//  r->headers_in.content_length
		// and the digital:
		//  r->headers_in.content_length_n
	}
	
	return JS_TRUE;
}


static JSBool
setProperty(JSContext *cx, JSObject *self, jsid rawid, JSBool strict, jsval *vp)
{
	u_char                     *key, *lowercased;
	size_t                      key_len;
	ngx_http_request_t         *r;
	ngx_table_elt_t            *header;
	ngx_http_header_t          *hh;
	ngx_table_elt_t           **phh;
	ngx_int_t                   hash;
	JSString                   *key_jsstr, *value_jsstr;
	jsval                       id;
	
	TRACE();
	GET_PRIVATE(r);
	
	if (JS_IdToValue(cx, rawid, &id))
	{
		return JS_FALSE;
	}
	
	if (JSVAL_IS_STRING(id))
	{
		key_jsstr = JSVAL_TO_STRING(id);
		key = js_str2c_str(cx, key_jsstr, r->pool, &key_len);
		if (key == NULL)
		{
			// forward exception if any
			return JS_FALSE;
		}
		
		
		if (key_len == 0)
		{
			// just return an undefined value
			return JS_TRUE;
		}
		
		lowercased = ngx_palloc(r->pool, key_len);
		if (lowercased == NULL)
		{
			JS_ReportOutOfMemory(cx);
			return JS_FALSE;
		}
		
		hash = hash_header_name(lowercased, key, key_len);
		
		header = NULL;
		
		hh = search_hashed_headers_in(r, hash, lowercased, key_len);
		if (hh != NULL)
		{
			// and this means its value was already cached in some field
			// of the r->headers_in stuct (hh->offset tells which)
			if (hh->offset)
			{
				// we got the element of the r->headers_in.headers
				// without brute forcing through all headers names
				phh = (ngx_table_elt_t **) ((char *) &r->headers_in + hh->offset);
				header = *phh;
				
				if (header == NULL)
				{
					header = ngx_list_push(&r->headers_in.headers);
					if (header == NULL)
					{
						JS_ReportOutOfMemory(cx);
						return JS_FALSE;
					}
					*phh = header;
				}
			}
		}
		
		if (header == NULL)
		{
			header = search_headers_in(r, key, key_len);
		}
		
		if (header == NULL)
		{
			header = ngx_list_push(&r->headers_in.headers);
			if (header == NULL)
			{
				JS_ReportOutOfMemory(cx);
				return JS_FALSE;
			}
		}
		
		
		// it may call GC or do other comlicated things like vp.toString()
		value_jsstr = JS_ValueToString(cx, *vp);
		if (value_jsstr == NULL)
		{
			// forward exception if any
			return JS_FALSE;
		}
		
		// mark header as enabled
		header->hash = 1;
		
		// set up header name
		header->key.data = (u_char *) key;
		header->key.len = key_len;
		
		// has to be set to proxy_pass without a crash
		// see this http://wiki.nginx.org/HeadersManagement#headers_in_and_proxy_pass
		header->lowcase_key = lowercased;
		
		if (!js_str2ngx_str(cx, value_jsstr, r->pool, &header->value))
		{
			// forward exception if any
			return JS_FALSE;
		}
		
		if (NCASE_COMPARE(header->key, "Content-Length") != 0)
		{
			jsdouble  dp;
			
			// it may call GC or do other comlicated things like vp.toString()
			if (!JS_ValueToNumber(cx, *vp, &dp))
			{
				// forward exception if any
				return JS_FALSE;
			}
			
			r->headers_in.content_length_n = (off_t) dp;
			r->headers_in.content_length = header;
			ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "headers_in.content_length_n = %O", r->headers_in.content_length_n);
		}
	}
	
	
	return JS_TRUE;
}

static JSBool
delProperty(JSContext *cx, JSObject *self, jsid id, jsval *vp)
{
	TRACE();
	return JS_TRUE;
}


JSPropertySpec ngx_http_js__nginx_headers_in__props[] =
{
	{"cookies",              1,          JSPROP_READONLY,   NULL, NULL},
	
	{"$contentLength",       100,        JSPROP_READONLY,   NULL, NULL},
	{"$contentLengthN",      101,        JSPROP_READONLY,   NULL, NULL},
	{"$range",               102,        JSPROP_READONLY,   NULL, NULL},
	{0, 0, 0, NULL, NULL}
};


JSFunctionSpec ngx_http_js__nginx_headers_in__funcs[] =
{
	// JS_FS("empty",       method_empty,          1, 0, 0),
	JS_FS_END
};

JSClass ngx_http_js__nginx_headers_in__class =
{
	"HeadersIn",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, delProperty, getProperty, setProperty,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

JSBool
ngx_http_js__nginx_headers_in__init(JSContext *cx, JSObject *global)
{
	JSObject    *nginxobj;
	jsval        vp;
	
	E(JS_GetProperty(cx, global, "Nginx", &vp), "global.Nginx is undefined or is not a function");
	nginxobj = JSVAL_TO_OBJECT(vp);
	
	ngx_http_js__nginx_headers_in__prototype = JS_InitClass(cx, nginxobj, NULL, &ngx_http_js__nginx_headers_in__class,  constructor, 0,
		ngx_http_js__nginx_headers_in__props, ngx_http_js__nginx_headers_in__funcs,  NULL, NULL);
	E(ngx_http_js__nginx_headers_in__prototype, "Can`t JS_InitClass(Nginx.HeadersIn)");
	
	return JS_TRUE;
}


static ngx_inline ngx_int_t
hash_header_name(u_char *lowercased, u_char *name, size_t len)
{
	ngx_uint_t                  i, hash;
	
	TRACE();
	
	// calculate a hash of header name
	hash = 0;
	for (i = 0; i < len; i++)
	{
		lowercased[i] = ngx_tolower(name[i]);
		hash = ngx_hash(hash, lowercased[i]);
	}
	
	return hash;
}


static ngx_inline ngx_http_header_t *
search_hashed_headers_in(ngx_http_request_t *r, ngx_int_t hash, u_char *lowercased, size_t len)
{
	ngx_http_core_main_conf_t  *cmcf;
	
	TRACE();
	
	// look in hashed headers
	
	// The layout of hashed headers is stored in ngx_http_core_module main config.
	// All the hashes, its offsets and handlers are precalculated at the configuration time
	// in the ngx_http_init_headers_in_hash() at ngx_http.c:432
	// with data from ngx_http_headers_in at ngx_http_request.c:80.
	cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module);
	
	// find the currents header description (ngx_http_header_t) by its hash
	return ngx_hash_find(&cmcf->headers_in_hash, hash, lowercased, len);
}

static ngx_table_elt_t *
search_headers_in(ngx_http_request_t *r, u_char *name, size_t len)
{
	ngx_list_part_t            *part;
	ngx_table_elt_t            *h;
	ngx_uint_t                  i;
	
	TRACE();
	ngx_assert(r);
	ngx_assert(name);
	
	part = &r->headers_in.headers.part;
	h = part->elts;
	
	// headers array may consist of more than one part
	// so loop throgh all of it
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
		
		// just compare names case insensitively
		if (len != h[i].key.len || ngx_strcasecmp(name, h[i].key.data) != 0)
		{
			continue;
		}
		
		// ta-da, we got one
		return &h[i];
	}
	
	// no plain header was found
	return NULL;
}

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#include <js/jsapi.h>

#include "../ngx_http_js_module.h"
#include "../strings_util.h"
#include "Request.h"

#include "../macroses.h"


JSObject *ngx_http_js__nginx_headers_in__prototype;
JSClass ngx_http_js__nginx_headers_in__class;
static JSClass* private_class = &ngx_http_js__nginx_headers_in__class;

static ngx_table_elt_t *
search_headers_in(ngx_http_request_t *r, char *name, u_int len);


JSObject *
ngx_http_js__nginx_headers_in__wrap(JSContext *cx, JSObject *request, ngx_http_request_t *r)
{
	JSObject                  *headers;
	ngx_http_js_ctx_t         *ctx;
	
	if (!(ctx = ngx_http_get_module_ctx(r, ngx_http_js_module)))
		return NULL;
	
	if (ctx->js_headers_in)
		return ctx->js_headers_in;
	
	headers = JS_NewObject(cx, &ngx_http_js__nginx_headers_in__class, ngx_http_js__nginx_headers_in__prototype, NULL);
	if (!headers)
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
	
	if (!ctx->js_headers_in)
		return;
	
	JS_SetPrivate(cx, ctx->js_headers_in, NULL);
	ctx->js_headers_in = NULL;
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
		
		header = search_headers_in(r, name, 0);
		
		if (header)
			*vp = STRING_TO_JSVAL(JS_NewStringCopyN(cx, (char *) header->value.data, header->value.len));
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
		
		header = search_headers_in(r, key, key_len);
		if (header)
		{
			header->key.data = (u_char*)key;
			header->key.len = key_len;
			E(js_str2ngx_str(cx, value_jsstr, r->pool, &header->value), "Can`t js_str2ngx_str(value_jsstr)");
			// LOG("by hash");
			return JS_TRUE;
		}
		
		
		header = ngx_list_push(&r->headers_in.headers);
		if (header)
		{
			header->hash = 1;
			
			header->key.data = (u_char*)key;
			header->key.len = key_len;
			E(js_str2ngx_str(cx, value_jsstr, r->pool, &header->value), "Can`t js_str2ngx_str(value_jsstr)");
			
			if (NCASE_COMPARE(header->key, "Content-Length"))
			{
				E(JSVAL_IS_INT(*vp), "the Content-Length value must be an Integer");
				r->headers_in.content_length_n = (off_t) JSVAL_TO_INT(*vp);
				r->headers_in.content_length = header;
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
delProperty(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	TRACE();
	return JS_TRUE;
}


JSPropertySpec ngx_http_js__nginx_headers_in__props[] =
{
	// {"uri",      REQUEST_URI,          JSPROP_READONLY,   NULL, NULL},
	{0, 0, 0, NULL, NULL}
};


JSFunctionSpec ngx_http_js__nginx_headers_in__funcs[] = {
    // {"empty",       method_empty,          1, 0, 0},
    {0, NULL, 0, 0, 0}
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


static ngx_table_elt_t *
search_headers_in(ngx_http_request_t *r, char *name, u_int len)
{
	ngx_http_core_main_conf_t  *cmcf;
	ngx_list_part_t            *part;
	ngx_http_header_t          *hh;
	ngx_table_elt_t            **ph, *h;
	u_char                     *lowcase_key;//, *cookie
	ngx_uint_t                  i, hash; // n, 
	
	TRACE();
	ngx_assert(r);
	ngx_assert(name);
	
	if (len == 0)
	{
		len = strlen(name);
		if (len == 0)
			return NULL;
	}
		
	// look in hashed headers
	
	lowcase_key = ngx_palloc(r->pool, len);
	if (lowcase_key == NULL)
		return NULL;
	
	hash = 0;
	for (i = 0; i < len; i++)
	{
		lowcase_key[i] = ngx_tolower(name[i]);
		hash = ngx_hash(hash, lowcase_key[i]);
	}
	
	
	cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module);
	
	hh = ngx_hash_find(&cmcf->headers_in_hash, hash, lowcase_key, len);
		
	if (hh)
	{
		if (hh->offset)
		{
			ph = (ngx_table_elt_t **) ((char *) &r->headers_in + hh->offset);
			
			return *ph;
		}
	}
	
	
	// look in all headers
	
	part = &r->headers_in.headers.part;
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



// check this from time to time
// http://emiller.info/lxr/http/source/http/ngx_http_request.h#L219
// 
// typedef struct {
//     ngx_list_t                        headers;
// 
//     ngx_table_elt_t                  *host;
//     ngx_table_elt_t                  *connection;
//     ngx_table_elt_t                  *if_modified_since;
//     ngx_table_elt_t                  *user_agent;
//     ngx_table_elt_t                  *referer;
//     ngx_table_elt_t                  *content_length;
//     ngx_table_elt_t                  *content_type;
// 
//     ngx_table_elt_t                  *range;
//     ngx_table_elt_t                  *if_range;
// 
//     ngx_table_elt_t                  *transfer_encoding;
// 
// #if (NGX_HTTP_GZIP)
//     ngx_table_elt_t                  *accept_encoding;
//     ngx_table_elt_t                  *via;
// #endif
// 
//     ngx_table_elt_t                  *authorization;
// 
//     ngx_table_elt_t                  *keep_alive;
// 
// #if (NGX_HTTP_PROXY || NGX_HTTP_REALIP)
//     ngx_table_elt_t                  *x_forwarded_for;
// #endif
// 
// #if (NGX_HTTP_REALIP)
//     ngx_table_elt_t                  *x_real_ip;
// #endif
// 
// #if (NGX_HTTP_HEADERS)
//     ngx_table_elt_t                  *accept;
//     ngx_table_elt_t                  *accept_language;
// #endif
// 
// #if (NGX_HTTP_DAV)
//     ngx_table_elt_t                  *depth;
//     ngx_table_elt_t                  *destination;
//     ngx_table_elt_t                  *overwrite;
//     ngx_table_elt_t                  *date;
// #endif
// 
//     ngx_str_t                         user;
//     ngx_str_t                         passwd;
// 
//     ngx_array_t                       cookies;
// 
//     size_t                            host_name_len;
//     off_t                             content_length_n;
//     time_t                            keep_alive_n;
// 
//     unsigned                          connection_type:2;
//     unsigned                          msie:1;
//     unsigned                          msie4:1;
//     unsigned                          opera:1;
//     unsigned                          gecko:1;
//     unsigned                          konqueror:1;
// } ngx_http_headers_in_t;


// Nginx.Headers class

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#include <js/jsapi.h>
#include <assert.h>

#include "../ngx_http_js_module.h"
#include "../strings_util.h"
#include "Request.h"
// #include "HeadersIn.h"

#include "../macroses.h"

//#define unless(a) if(!(a))
#define JS_HEADER_IN_ROOT_NAME      "Nginx.HeadersIn instance"


JSObject *ngx_http_js__nginx_headers_out__prototype;
JSClass ngx_http_js__nginx_headers_out__class;


JSObject *
ngx_http_js__nginx_headers_out__wrap(JSContext *cx, ngx_http_request_t *r)
{
	LOG2("ngx_http_js__nginx_headers_out__wrap()");
	JSObject                  *headers;
	ngx_http_js_ctx_t         *ctx;
	
	if (!(ctx = ngx_http_get_module_ctx(r, ngx_http_js_module)))
		ngx_http_js__nginx_request__wrap(cx, r);
	
	if (ctx->js_headers)
		return ctx->js_headers;
	
	headers = JS_NewObject(cx, &ngx_http_js__nginx_headers_out__class, ngx_http_js__nginx_headers_out__prototype, NULL);
	if (!headers)
	{
		JS_ReportOutOfMemory(cx);
		return NULL;
	}
	
	if (!JS_AddNamedRoot(cx, &ctx->js_headers, JS_HEADER_IN_ROOT_NAME))
	{
		JS_ReportError(cx, "Can`t add new root %s", JS_HEADER_IN_ROOT_NAME);
		return NULL;
	}
	
	JS_SetPrivate(cx, headers, r);
	
	ctx->js_headers = headers;
	
	return headers;
}


void
ngx_http_js__nginx_headers_out__cleanup(JSContext *cx, ngx_http_request_t *r, ngx_http_js_ctx_t *ctx)
{
	LOG2("ngx_http_js__nginx_headers_out__wrap()");
	
	assert(ctx);
	
	if (!ctx->js_headers)
		return;
	
	if (!JS_RemoveRoot(cx, &ctx->js_headers))
		JS_ReportError(cx, "Can`t remove cleaned up root %s", JS_HEADER_IN_ROOT_NAME);
	
	JS_SetPrivate(cx, ctx->js_headers, NULL);
	ctx->js_headers = NULL;
}


static JSBool
method_empty(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
{
	LOG2("Nginx.Headers#empty");
	ngx_http_request_t  *r;
	
	GET_PRIVATE();
	
	E(argc == 1 && JSVAL_IS_STRING(argv[0]), "Nginx.Headers#empty takes 1 argument: str:String");
	
	return JS_TRUE;
}



static JSBool
constructor(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
{
	return JS_TRUE;
}


// enum propid { HEADER_LENGTH };

static JSBool
getProperty(JSContext *cx, JSObject *this, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	char                       *name;
	ngx_http_core_main_conf_t  *cmcf;
	ngx_list_part_t            *part;
	ngx_http_header_t          *hh;
	ngx_table_elt_t            **ph, *h;
	u_char                     *lowcase_key;//, *cookie
	ngx_uint_t                  i, hash; // n, 
	u_int                       len;
	
	GET_PRIVATE();
	
	if (JSVAL_IS_INT(id))
	{
		switch (JSVAL_TO_INT(id))
		{
			// case REQUEST_URI:
			// *vp = STRING_TO_JSVAL(JS_NewStringCopyN(cx, (char *) r->uri.data, r->uri.len));
			// break;
			
			
		}
	}
	else if (JSVAL_IS_STRING(id) && (name = JS_GetStringBytes(JSVAL_TO_STRING(id))) != NULL)
	{
		// if (!strcmp(member_name, "constructor"))
		len = strlen(name);
		LOG("getProperty: %s, len: %u", name, len);
		
		
		// look in hashed headers
		
		lowcase_key = ngx_palloc(r->pool, len);
		if (lowcase_key == NULL)
		{
			JS_ReportOutOfMemory(cx);
			return JS_FALSE;
		}
		
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
				
				if (*ph)
					*vp = STRING_TO_JSVAL(JS_NewStringCopyN(cx, (char *) (*ph)->value.data, (*ph)->value.len));
				
				return JS_TRUE;
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
			
			if (len != h[i].key.len || ngx_strcasecmp((u_char *) name, h[i].key.data) != 0)
				continue;
			
			*vp = STRING_TO_JSVAL(JS_NewStringCopyN(cx, (char *) h[i].value.data, h[i].value.len));
			
			return JS_TRUE;
		}
	}
	
	return JS_TRUE;
}

JSPropertySpec ngx_http_js__nginx_headers_out__props[] =
{
	// {"uri",      REQUEST_URI,          JSPROP_READONLY,   NULL, NULL},
	{0, 0, 0, NULL, NULL}
};


JSFunctionSpec ngx_http_js__nginx_headers_out__funcs[] = {
    // {"empty",       method_empty,          1, 0, 0},
    {0, NULL, 0, 0, 0}
};

JSClass ngx_http_js__nginx_headers_out__class =
{
	"Headers",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub, getProperty, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

JSBool
ngx_http_js__nginx_headers_out__init(JSContext *cx)
{
	JSObject    *nginxobj;
	JSObject    *global;
	jsval        vp;
	
	global = JS_GetGlobalObject(cx);
	
	E(JS_GetProperty(cx, global, "Nginx", &vp), "global.Nginx is undefined or is not a function");
	nginxobj = JSVAL_TO_OBJECT(vp);
	
	ngx_http_js__nginx_headers_out__prototype = JS_InitClass(cx, nginxobj, NULL, &ngx_http_js__nginx_headers_out__class,  constructor, 0,
		ngx_http_js__nginx_headers_out__props, ngx_http_js__nginx_headers_out__funcs,  NULL, NULL);
	E(ngx_http_js__nginx_headers_out__prototype, "Can`t JS_InitClass(Nginx.Headers)");
	
	return JS_TRUE;
}


// Nginx.HeadersOut class

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#include <ngx_http_jsapi.h>

#include <ngx_http_js_module.h>
#include <nginx_js_glue.h>
#include <strings_util.h>
#include <classes/Chain.h>

#include <nginx_js_macroses.h>



JSObject *ngx_http_js__nginx_chain__prototype;
JSClass ngx_http_js__nginx_chain__class;
static JSClass *private_class = &ngx_http_js__nginx_chain__class;


JSObject *
ngx_http_js__nginx_chain__wrap(JSContext *cx, ngx_chain_t *ch, JSObject *request)
{
	TRACE();
	JSObject                  *chain;
	
	chain = JS_NewObject(cx, &ngx_http_js__nginx_chain__class, ngx_http_js__nginx_chain__prototype, NULL);
	if (!chain)
	{
		JS_ReportOutOfMemory(cx);
		return NULL;
	}
	
	JS_SetPrivate(cx, chain, ch);
	JS_SetReservedSlot(cx, chain, NGX_JS_CHAIN_SLOT__REQUEST, OBJECT_TO_JSVAL(request));
	
	return chain;
}


static JSBool
method_toString(JSContext *cx, uintN argc, jsval *vp)
{
	TRACE();
	ngx_chain_t  *ch, *next;
	int           len;
	char         *buff;
	JSObject     *self;
	jsval         rval;
	
	GET_PRIVATE(ch);
	
	if (ch->buf && !ch->next)
	{
		DATA_LEN_to_JS_STRING_to_JSVAL(cx, ch->buf->pos, ch->buf->last - ch->buf->pos, rval);
	}
	else
	{
		len = 0;
		for (next=ch; next; next = next->next)
		{
			if (next->buf)
				len += next->buf->last - next->buf->pos;
		}
		
		buff = JS_malloc(cx, len);
		E(buff, "Can`t JS_malloc");
		
		len = 0;
		for (next=ch; next; next = next->next)
		{
			if (next->buf)
			{
				ngx_memcpy(&buff[len], next->buf->pos, next->buf->last - next->buf->pos);
				len += next->buf->last - next->buf->pos;
			}
		}
		
		DATA_LEN_to_JS_STRING_to_JSVAL(cx, buff, len, rval);
		
		JS_free(cx, buff);
	}
	
	JS_SET_RVAL(cx, vp, rval);
	return JS_TRUE;
}



static JSBool
constructor(JSContext *cx, uintN argc, jsval *vp)
{
	TRACE();
	JS_ReportError(cx, "Nginx.Chain instance can not be created at JS side");
	return JS_FALSE;
}


// enum propid { HEADER_LENGTH };


JSPropertySpec ngx_http_js__nginx_chain__props[] =
{
	// {"uri",      REQUEST_URI,          JSPROP_READONLY,   NULL, NULL},
	{0, 0, 0, NULL, NULL}
};


JSFunctionSpec ngx_http_js__nginx_chain__funcs[] = {
    JS_FS("toString",       method_toString,          0, 0),
    JS_FS_END
};

JSClass ngx_http_js__nginx_chain__class =
{
	"Chain",
	JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(NGX_JS_CHAIN_SLOTS_COUNT),
	JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

JSBool
ngx_http_js__nginx_chain__init(JSContext *cx, JSObject *global)
{
	JSObject    *nginxobj;
	jsval        vp;
	
	E(JS_GetProperty(cx, global, "Nginx", &vp), "global.Nginx is undefined");
	nginxobj = JSVAL_TO_OBJECT(vp);
	
	ngx_http_js__nginx_chain__prototype = JS_InitClass(cx, nginxobj, NULL, &ngx_http_js__nginx_chain__class,  constructor, 0,
		ngx_http_js__nginx_chain__props, ngx_http_js__nginx_chain__funcs,  NULL, NULL);
	E(ngx_http_js__nginx_chain__prototype, "Can`t JS_InitClass(Nginx.Chain)");
	
	return JS_TRUE;
}

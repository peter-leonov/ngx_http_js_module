#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#include <js/jsapi.h>

#include <strings_util.h>
#include <nginx_js_macroses.h>

#define MAX_STRLEN NGX_MAX_SIZE_T_VALUE

ngx_buf_t *
js_str2ngx_buf(JSContext *cx, JSString *str, ngx_pool_t *pool)
{
	ngx_buf_t           *b;
	const char          *p;
	size_t               len;
	
	ngx_assert(cx);
	ngx_assert(str);
	ngx_assert(pool);
	
	// get bytes with UTF-16 -> UTF-8 conversion
	p = JS_GetStringBytes(str);
	// JS_GetStringBytes() returns an empty C-string on failure,
	// but on success it does the same, so treat every response as a success
	if (p == NULL)
	{
		return NULL;
	}
	
	// run through the UTF-8 string again
	len = ngx_strlen(p);
	
	// allocate another buffer for the string, the third:
	// the first for the original string,
	// the second for its UTF-8 representation
	b = ngx_create_temp_buf(pool, len);
	if (b == NULL)
		return NULL;
	
	ngx_memcpy(b->last, p, len);
	b->last = b->last + len;
	
	return b;
}

JSBool
js_str2ngx_str(JSContext *cx, JSString *str, ngx_pool_t *pool, ngx_str_t *s)
{
	const u_char        *p;
	size_t               len;
	
	ngx_assert(cx);
	ngx_assert(str);
	ngx_assert(pool);
	ngx_assert(s);
	
	// get bytes with UTF-16 -> UTF-8 conversion
	p = (u_char *) JS_GetStringBytes(str);
	// JS_GetStringBytes() returns an empty C-string on failure,
	// but on success it does the same, so treat every response as a success
	if (p == NULL)
	{
		return JS_FALSE;
	}
	
	// run through the UTF-8 string again
	len = ngx_strlen(p);
	
	// allocate another buffer for the string (the third, again!!!)
	s->data = ngx_palloc(pool, len+1);
	if (s->data == NULL)
	{
		return JS_FALSE;
	}
	
	// copy a string memory (the second time!)
	ngx_memcpy(s->data, p, len);
	s->data[len] = 0;
	s->len = len;
	
	// all this was very uneffective,
	// finally, exiting
	return JS_TRUE;
}


u_char *
js_str2c_str(JSContext *cx, JSString *str, ngx_pool_t *pool, size_t *out_len)
{
	u_char              *p, *np;
	size_t               len;
	
	ngx_assert(cx);
	ngx_assert(str);
	ngx_assert(pool);
	
	p = (u_char *) JS_GetStringBytes(str);
	// JS_GetStringBytes() returns an empty C-string on failure,
	// but on success it does the same, so treat every response as a success
	if (p == NULL)
	{
		return NULL;
	}
	
	len = ngx_strlen(p);
	
	np = ngx_palloc(pool, len+1);
	if (np == NULL)
	{
		return NULL;
	}
	
	ngx_memcpy(np, p, len);
	np[len] = 0;
	
	if (out_len != NULL)
		*out_len = len;
	
	return np;
}

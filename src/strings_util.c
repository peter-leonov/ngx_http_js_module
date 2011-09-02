#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#include <ngx_http_jsapi.h>

#include <strings_util.h>
#include <nginx_js_macroses.h>

#define MAX_STRLEN NGX_MAX_SIZE_T_VALUE

ngx_buf_t *
js_str2ngx_buf(JSContext *cx, JSString *str, ngx_pool_t *pool)
{
	ngx_buf_t           *b;
	size_t               len;
	
	ngx_assert(cx);
	ngx_assert(str);
	ngx_assert(pool);
	
	// calculate the length in bytes of the UTF-8 string representation
	// on error JS_GetStringEncodingLength() will report the broken character code
	len = JS_GetStringEncodingLength(cx, str);
	if (len == (size_t) -1)
	{
		return NULL;
	}
	
	// allocate a buffer for the string
	b = ngx_create_temp_buf(pool, len);
	if (b == NULL)
	{
		return NULL;
	}
	
	// get bytes with UTF-16 to UTF-8 conversion
	JS_EncodeStringToBuffer(str, (char *) b->last, len);
	b->last = b->last + len;
	
	return b;
}

JSBool
js_str2ngx_str(JSContext *cx, JSString *str, ngx_pool_t *pool, ngx_str_t *s)
{
	size_t               len;
	
	ngx_assert(cx);
	ngx_assert(str);
	ngx_assert(pool);
	ngx_assert(s);
	
	// calculate the length in bytes of the UTF-8 string representation
	// on error JS_GetStringEncodingLength() will report the broken character code
	len = JS_GetStringEncodingLength(cx, str);
	if (len == (size_t) -1)
	{
		return JS_FALSE;
	}
	
	// allocate a buffer for the string
	s->data = ngx_palloc(pool, len + 1);
	if (s->data == NULL)
	{
		return JS_FALSE;
	}
	
	// get bytes with UTF-16 to UTF-8 conversion
	JS_EncodeStringToBuffer(str, (char *) s->data, len);
	s->data[len] = 0;
	s->len = len;
	
	return JS_TRUE;
}


u_char *
js_str2c_str(JSContext *cx, JSString *str, ngx_pool_t *pool, size_t *out_len)
{
	u_char              *p;
	size_t               len;
	
	ngx_assert(cx);
	ngx_assert(str);
	ngx_assert(pool);
	
	// calculate the length in bytes of the UTF-8 string representation
	// on error JS_GetStringEncodingLength() will report the broken character code
	len = JS_GetStringEncodingLength(cx, str);
	if (len == (size_t) -1)
	{
		return NULL;
	}
	
	p = ngx_palloc(pool, len + 1);
	if (p == NULL)
	{
		return NULL;
	}
	
	// get bytes with UTF-16 to UTF-8 conversion
	JS_EncodeStringToBuffer(str, (char *) p, len);
	p[len] = 0;
	
	if (out_len != NULL)
		*out_len = len;
	
	return p;
}


u_char *
js_debug_value_to_cstring(JSContext *cx, jsval v)
{
	JSString  *str;
	u_char    *p;
	
	str = JS_ValueToString(cx, v);
	if (str == NULL)
	{
		return (u_char *) "[JS_ValueToString() == NULL]";
	}
	
	p = (u_char *) JS_EncodeString(cx, str);
	if (p == NULL)
	{
		return (u_char *) "[JS_GetStringBytes() == NULL]";
	}
	
	return p;
}

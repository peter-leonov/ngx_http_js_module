#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>
#include <assert.h>

#include <js/jsapi.h>


ngx_buf_t *
js_str2ngx_buf(JSContext *cx, JSString *str, ngx_pool_t *pool, size_t len)
{
	ngx_buf_t           *b;
	const char          *p;
	
	if (len == 0)
		len = JS_GetStringLength(str);
	
	p = JS_GetStringBytes(str);
	if (p == NULL)
		return NULL;
	
	b = ngx_create_temp_buf(pool, len);
	if (b == NULL)
		return NULL;
	ngx_memcpy(b->last, p, len);
	b->last = b->last + len;
	
	return b;
}

JSBool
js_str2ngx_str(JSContext *cx, JSString *str, ngx_pool_t *pool, ngx_str_t *s, size_t len)
{
	const char          *p;
	
	s->len = 0;
	s->data = NULL;
	
	if (len == 0)
		if ((len = JS_GetStringLength(str)) == 0)
			return JS_TRUE;
	
	p = JS_GetStringBytes(str);
	if (p == NULL)
	{
		JS_ReportOutOfMemory(cx);
		return JS_FALSE;
	}
	
	s->data = ngx_palloc(pool, len);
	if (s->data == NULL)
	{
		JS_ReportOutOfMemory(cx);
		return JS_FALSE;
	}
	
	ngx_memcpy(s->data, p, len);
	s->len = len;
	
	return JS_TRUE;
}

// JSBool
// hash_find_string(ngx_hash_t *hashp, char *name, u_int len, ngx_pool_t *pool)
// {
// 	ngx_uint_t                  i, hash;
// 	u_char                     *lowcase_key;//, *cookie
// 		
// 	assert(hashp);
// 	assert(name);
// 	assert(pool);
// 	
// 	lowcase_key = ngx_palloc(pool, len);
// 	if (lowcase_key == NULL)
// 		return JS_FALSE;
// 	
// 	hash = 0;
// 	for (i = 0; i < len; i++)
// 	{
// 		lowcase_key[i] = ngx_tolower(name[i]);
// 		hash = ngx_hash(hash, lowcase_key[i]);
// 	}
// 	
// 	
// 	cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module);
// 	
// 	hh = ngx_hash_find(hashp, hash, lowcase_key, len);
// 	
// 	if (hh)
// 	{
// 		if (hh->offset)
// 		{
// 			ph = (ngx_table_elt_t **) ((char *) &r->headers_in + hh->offset);
// 			
// 			if (*ph)
// 				*vp = STRING_TO_JSVAL(JS_NewStringCopyN(cx, (char *) (*ph)->value.data, (*ph)->value.len));
// 			
// 			return JS_TRUE;
// 		}
// 	}
// }
// 
// JSBool
// list_find_string(ngx_hash_t *hashp, char *name, u_int len, ngx_pool_t *pool)
// {
// 	// look in all headers
// 	
// 	part = &r->headers_in.headers.part;
// 	h = part->elts;
// 	
// 	for (i = 0; /* void */ ; i++)
// 	{
// 		if (i >= part->nelts)
// 		{
// 			if (part->next == NULL)
// 				break;
// 			
// 			part = part->next;
// 			h = part->elts;
// 			i = 0;
// 		}
// 		
// 		if (len != h[i].key.len || ngx_strcasecmp((u_char *) name, h[i].key.data) != 0)
// 			continue;
// 		
// 		*vp = STRING_TO_JSVAL(JS_NewStringCopyN(cx, (char *) h[i].value.data, h[i].value.len));
// 		
// 		return JS_TRUE;
// 	}
// }

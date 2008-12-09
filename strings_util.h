
#ifndef _NGX_HTTP_JS_STRINGS_UTIL_JS_H_INCLUDED_
#define _NGX_HTTP_JS_STRINGS_UTIL_JS_H_INCLUDED_

extern ngx_buf_t *
js_str2ngx_buf(JSContext *cx, JSString *str, ngx_pool_t *pool, size_t len);

extern ngx_int_t
js_str2ngx_str(JSContext *cx, JSString *str, ngx_pool_t *pool, ngx_str_t *s, size_t len);

#endif

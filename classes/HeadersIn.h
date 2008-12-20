
#ifndef _NGX_HTTP_JS_NGINX_HEADERS_IN_H_INCLUDED_
#define _NGX_HTTP_JS_NGINX_HEADERS_IN_H_INCLUDED_

extern JSClass ngx_http_js__nginx_headers_in__class;
extern JSObject *ngx_http_js__nginx_headers_in__prototype;

extern JSPropertySpec ngx_http_js__nginx_headers_in__props;
extern JSFunctionSpec ngx_http_js__nginx_headers_in__funcs;

extern JSBool
ngx_http_js__nginx_headers_in__init(JSContext *cx);

extern JSObject *
ngx_http_js__nginx_headers_in__wrap(JSContext *cx, ngx_http_request_t *r);

extern void
ngx_http_js__nginx_headers_in__cleanup(JSContext *cx, ngx_http_request_t *r, ngx_http_js_ctx_t *ctx);


#endif

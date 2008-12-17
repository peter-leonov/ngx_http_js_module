
#ifndef _NGX_HTTP_JS_NGINX_HEADERS_H_INCLUDED_
#define _NGX_HTTP_JS_NGINX_HEADERS_H_INCLUDED_

extern JSClass ngx_http_js__nginx_headers__class;
extern JSObject *ngx_http_js__nginx_headers__prototype;

extern JSPropertySpec ngx_http_js__nginx_headers__props;
extern JSFunctionSpec ngx_http_js__nginx_headers__funcs;

extern JSBool
ngx_http_js__nginx_headers__init(JSContext *cx);

extern JSObject *
ngx_http_js__nginx_headers__wrap(JSContext *cx, ngx_http_request_t *r);

extern void
ngx_http_js__nginx_headers__cleanup(JSContext *cx, ngx_http_request_t *r, ngx_http_js_ctx_t *ctx);


#endif

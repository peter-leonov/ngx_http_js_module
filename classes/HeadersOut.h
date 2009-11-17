#ifndef _NGX_HTTP_JS_NGINX_HEADERS_OUT_H_INCLUDED_
#define _NGX_HTTP_JS_NGINX_HEADERS_OUT_H_INCLUDED_

extern JSClass ngx_http_js__nginx_headers_out__class;
extern JSObject *ngx_http_js__nginx_headers_out__prototype;

extern JSPropertySpec ngx_http_js__nginx_headers_out__props;
extern JSFunctionSpec ngx_http_js__nginx_headers_out__funcs;

extern JSBool
ngx_http_js__nginx_headers_out__init(JSContext *cx);

extern JSObject *
ngx_http_js__nginx_headers_out__wrap(JSContext *cx, ngx_http_request_t *r);

extern void
ngx_http_js__nginx_headers_out__cleanup(JSContext *cx, ngx_http_request_t *r, ngx_http_js_ctx_t *ctx);


#endif
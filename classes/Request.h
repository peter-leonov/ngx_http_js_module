#ifndef _NGX_HTTP_JS_NGINX_REQUEST_H_INCLUDED_
#define _NGX_HTTP_JS_NGINX_REQUEST_H_INCLUDED_


#define JS_REQUEST_ROOT_NAME               "Nginx.Request instance"

#define NGX_JS_REQUEST_SLOT__HAS_BODY_CALLBACK        0
#define NGX_JS_REQUEST_SLOT__SET_TIMER                1
#define NGX_JS_REQUEST_SLOT__SUBREQUEST_CALLBACK      2
#define NGX_JS_REQUEST_SLOT__HEADERS_IN               3
#define NGX_JS_REQUEST_SLOT__HEADERS_OUT              4
#define NGX_JS_REQUEST_SLOT__COOKIES                  5
#define NGX_JS_REQUEST_SLOTS_COUNT                    6


extern JSClass ngx_http_js__nginx_request__class;
extern JSObject *ngx_http_js__nginx_request__prototype;

extern JSPropertySpec ngx_http_js__nginx_request__props[];
extern JSFunctionSpec ngx_http_js__nginx_request__funcs[];

extern JSBool
ngx_http_js__nginx_request__init(JSContext *cx, JSObject *global);

extern JSObject *
ngx_http_js__nginx_request__wrap(JSContext *cx, ngx_http_request_t *r);

extern void
ngx_http_js__nginx_request__cleanup(ngx_http_js_ctx_t *ctx, ngx_http_request_t *r, JSContext *cx);

extern ngx_int_t
ngx_http_js__nginx_request__root_in(JSContext *cx, ngx_http_request_t *r, JSObject *request);


#endif

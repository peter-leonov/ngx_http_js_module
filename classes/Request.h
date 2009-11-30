#ifndef _NGX_HTTP_JS_NGINX_REQUEST_H_INCLUDED_
#define _NGX_HTTP_JS_NGINX_REQUEST_H_INCLUDED_


#define JS_REQUEST_ROOT_NAME               "Nginx.Request instance"

#define NGX_JS_REQUEST_SLOT__HAS_BODY_CALLBACK        0
#define NGX_JS_REQUEST_SLOT__SET_TIMEOUT              1
#define NGX_JS_REQUEST_SLOT__SUBREQUEST_CALLBACK      2
#define NGX_JS_REQUEST_SLOTS_COUNT                    3


extern JSClass ngx_http_js__nginx_request__class;
extern JSObject *ngx_http_js__nginx_request__prototype;

extern JSPropertySpec ngx_http_js__nginx_request__props[];
extern JSFunctionSpec ngx_http_js__nginx_request__funcs[];

extern JSBool
ngx_http_js__nginx_request__init(JSContext *cx);

extern JSObject *
ngx_http_js__nginx_request__wrap(JSContext *cx, ngx_http_request_t *r);


#endif
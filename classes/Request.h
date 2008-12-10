
#ifndef _NGX_HTTP_JS_NGINX_REQUEST_H_INCLUDED_
#define _NGX_HTTP_JS_NGINX_REQUEST_H_INCLUDED_

extern JSClass ngx_http_js__nginx_request_class;
extern JSObject *ngx_http_js__nginx_request_prototype;

extern JSPropertySpec ngx_http_js__nginx_request_props;
extern JSFunctionSpec ngx_http_js__nginx_request_class_funcs;

extern JSBool
ngx_http_js__nginx_request__init(JSContext *cx);

extern JSObject *
ngx_http_js__wrap_nginx_request(JSContext *cx, ngx_http_request_t *r);


#endif

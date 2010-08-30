#ifndef _NGX_HTTP_JS_NGINX_REQUEST_H_INCLUDED_
#define _NGX_HTTP_JS_NGINX_REQUEST_H_INCLUDED_


#define JS_REQUEST_ROOT_NAME               "Nginx.Request instance"

#define NGX_JS_REQUEST_SLOT__HAS_BODY_CALLBACK        0
#define NGX_JS_REQUEST_SLOT__SET_TIMER                1
#define NGX_JS_REQUEST_SLOT__SUBREQUEST_CALLBACK      2
#define NGX_JS_REQUEST_SLOT__HEADERS_IN               3
#define NGX_JS_REQUEST_SLOT__HEADERS_OUT              4
#define NGX_JS_REQUEST_SLOT__COOKIES                  5
#define NGX_JS_REQUEST_SLOT__VARIABLES                6
#define NGX_JS_REQUEST_SLOT__CLEANUP                  7
#define NGX_JS_REQUEST_SLOTS_COUNT                    8

#ifndef ngx_str_set
#define ngx_str_set(str, text)                                               \
    (str)->len = sizeof(text) - 1; (str)->data = (u_char *) text
#endif

#ifndef ngx_str_null
#define ngx_str_null(str)   (str)->len = 0; (str)->data = NULL
#endif

extern JSClass ngx_http_js__nginx_request__class;
extern JSObject *ngx_http_js__nginx_request__prototype;

extern JSPropertySpec ngx_http_js__nginx_request__props[];
extern JSFunctionSpec ngx_http_js__nginx_request__funcs[];

JSBool
ngx_http_js__nginx_request__init(JSContext *cx, JSObject *global);

JSObject *
ngx_http_js__nginx_request__wrap(JSContext *cx, ngx_http_request_t *r);

JSBool
ngx_http_js__request__call_function(JSContext *cx, ngx_http_request_t *r, JSObject *function, jsval *rval);


#endif

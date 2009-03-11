
#ifndef _NGX_HTTP_JS_NGINX_CHAIN_H_INCLUDED_
#define _NGX_HTTP_JS_NGINX_CHAIN_H_INCLUDED_

extern JSClass ngx_http_js__nginx_chain__class;
extern JSObject *ngx_http_js__nginx_chain__prototype;

// extern JSPropertySpec ngx_http_js__nginx_chain__props;
// extern JSFunctionSpec ngx_http_js__nginx_chain__funcs;

extern JSBool
ngx_http_js__nginx_chain__init(JSContext *cx);

extern JSObject *
ngx_http_js__nginx_chain__wrap(JSContext *cx, ngx_chain_t *ch, JSObject *request);

#define NGX_HTTP_JS_CHAIN_REQUEST_SLOT 0


#endif

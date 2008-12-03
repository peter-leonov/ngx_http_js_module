
/*
 * Copyright (C) Peter Leonov
 */


#ifndef _NGX_HTTP_JS_MODULE_H_INCLUDED_
#define _NGX_HTTP_JS_MODULE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#define XP_UNIX
#include <js/jsapi.h>

typedef ngx_http_request_t   *nginx;

typedef struct {
    ngx_str_t                 filename;
    ngx_str_t                 redirect_uri;
    ngx_str_t                 redirect_args;

    JSObject                 *next;

    ngx_uint_t                done;       /* unsigned  done:1; */


#if (NGX_HTTP_SSI)
    ngx_http_ssi_ctx_t       *ssi;
#endif
} ngx_http_js_ctx_t;

typedef struct {
    JSObject          *sub;
    ngx_str_t          handler;
} ngx_http_js_loc_conf_t;

typedef struct {
    JSContext   *js;
    JSObject    *global;
    // HV                *nginx;
    ngx_array_t        requires;
} ngx_http_js_main_conf_t;

typedef struct {
	ngx_conf_t *cf;
	ngx_http_js_main_conf_t *jsmcf;
	ngx_log_t *log;
} ngx_http_js_context_private_t;


extern ngx_module_t  ngx_http_js_module;

void ngx_http_perl_handle_request(ngx_http_request_t *r);



#endif /* _NGX_HTTP_JS_MODULE_H_INCLUDED_ */

#ifndef _NGX_HTTP_JS_MODULE_H_INCLUDED_
#define _NGX_HTTP_JS_MODULE_H_INCLUDED_

typedef ngx_http_request_t   *nginx;

typedef struct {
	ngx_str_t                 redirect_uri;
	ngx_str_t                 redirect_args;
	
	JSObject                 *js_request;
	JSObject                 *js_headers_in;
	JSObject                 *js_headers_out;
	
	int                       filter_enabled;
	ngx_chain_t              *chain_first, *chain_last;
	
	ngx_event_t               js_timer;
} ngx_http_js_ctx_t;



typedef struct {
	JSObject                  *handler_function;
	ngx_str_t                  handler_name;
	JSObject                  *filter_function;
	ngx_str_t                  filter_name;
	ngx_array_t               *filter_types;
} ngx_http_js_loc_conf_t;



typedef struct{
	size_t                     maxmem;
	ngx_array_t                requires, loads;
} ngx_http_js_main_conf_t;



extern ngx_http_output_header_filter_pt  ngx_http_js_next_header_filter;
extern ngx_http_output_body_filter_pt    ngx_http_js_next_body_filter;

extern ngx_module_t  ngx_http_js_module;

#endif
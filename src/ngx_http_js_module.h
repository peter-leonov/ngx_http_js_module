#ifndef _NGX_HTTP_JS_MODULE_H_INCLUDED_
#define _NGX_HTTP_JS_MODULE_H_INCLUDED_

typedef ngx_http_request_t   *nginx;

typedef struct {
	ngx_str_t                 redirect_uri;
	ngx_str_t                 redirect_args;
	
	JSObject                 *js_request;
	JSObject                 *js_headers_in;
	JSObject                 *js_headers_out;
	JSObject                 *js_cookies;
	JSObject                 *js_variables;
	
	int                       js_cleanup_handler_set:1;
	int                       filter_enabled:1;
	ngx_chain_t              *chain_first, *chain_last;
	
	ngx_event_t               js_timer;
} ngx_http_js_ctx_t;


typedef struct {
	JSObject                  *content_handler_function;
	ngx_str_t                  content_handler_name;
	JSObject                  *filter_function;
	ngx_str_t                  filter_name;
	ngx_array_t               *filter_types;
} ngx_http_js_loc_conf_t;


typedef struct{
	size_t                     maxmem;
	ngx_array_t                requires, loads;
} ngx_http_js_main_conf_t;


typedef struct
{
	JSObject                  *function;
	ngx_str_t                  handler;
} ngx_http_js_variable_t;


extern ngx_http_output_header_filter_pt  ngx_http_js_next_header_filter;
extern ngx_http_output_body_filter_pt    ngx_http_js_next_body_filter;

extern ngx_module_t  ngx_http_js_module;

#endif

#ifndef _NGX_HTTP_JS_MODULE_H_INCLUDED_
#define _NGX_HTTP_JS_MODULE_H_INCLUDED_

typedef ngx_http_request_t   *nginx;

typedef struct {
	ngx_str_t                 filename;
	ngx_str_t                 redirect_uri;
	ngx_str_t                 redirect_args;
	
	// void                 *next; // JSObject
	void                     *js_cx; // JSContext
	void                     *js_request; // JSObject
	void                     *js_request_callback; // JSObject
	void                     *js_headers_in; // JSObject
	void                     *js_headers_out; // JSObject
	void                     *js_has_body_callback; // JSObject
	
	int                       filter_enabled;
	
	ngx_event_t               js_timer;
	// ngx_uint_t                done;       /* unsigned  done:1; */
} ngx_http_js_ctx_t;



typedef struct {
	void                      *handler_function; // JSObject
	ngx_str_t                  handler_name;
	void                      *filter_function; // JSObject;
	ngx_str_t                  filter_name;
	ngx_array_t               *filter_types;
} ngx_http_js_loc_conf_t;



typedef struct{
	void                      *js_cx; // JSContext
	void                      *js_global; // JSObject
	ngx_array_t                requires, loads;
} ngx_http_js_main_conf_t;



extern ngx_http_output_header_filter_pt  ngx_http_js_next_header_filter;
extern ngx_http_output_body_filter_pt    ngx_http_js_next_body_filter;

extern ngx_module_t  ngx_http_js_module;

#endif
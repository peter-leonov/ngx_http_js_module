#ifndef _NGX_HTTP_JS_GLUE_H_INCLUDED_
#define _NGX_HTTP_JS_GLUE_H_INCLUDED_

extern JSRuntime *ngx_http_js_module_js_runtime;
extern JSContext *ngx_http_js_module_js_context;
extern JSObject  *ngx_http_js_module_js_global;
extern ngx_log_t *ngx_http_js_module_log;

#define js_cx ngx_http_js_module_js_context
#define js_global ngx_http_js_module_js_global

extern char *
ngx_http_js__glue__set_handler(ngx_conf_t *cf, JSObject **handler, const char *root_name);

extern char *
ngx_http_js__glue__js_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

ngx_int_t
ngx_http_js__glue__access_handler(ngx_http_request_t *r);

ngx_int_t
ngx_http_js__glue__content_handler(ngx_http_request_t *r);

extern char *
ngx_http_js__glue__init_interpreter(ngx_conf_t *cf);

extern ngx_int_t
ngx_http_js__glue__init_worker(ngx_cycle_t *cycle);

extern void
ngx_http_js__glue__exit_worker(ngx_cycle_t *cycle);

extern void
ngx_http_js__glue__exit_master(ngx_cycle_t *cycle);

#endif

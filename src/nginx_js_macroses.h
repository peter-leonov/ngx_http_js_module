#ifndef _NGX_HTTP_JS_MACROSES_INCLUDED_
#define _NGX_HTTP_JS_MACROSES_INCLUDED_

#if (NGX_HTTP_JS_COLORED)
#define COLOR_CLEAR   "\x1B[0m"
#define COLOR_BRIGHT  "\x1B[1m"
#define COLOR_FAINT   "\x1B[2m"

#define COLOR_BLACK   "\x1B[30m"
#define COLOR_RED     "\x1B[31m"
#define COLOR_GREEN   "\x1B[32m"
#define COLOR_YELLOW  "\x1B[33m"
#define COLOR_BLUE    "\x1B[34m"
#define COLOR_MAGENTA "\x1B[35m"
#define COLOR_CYAN    "\x1B[36m"
#define COLOR_WHITE   "\x1B[37m"
#else
#define COLOR_CLEAR   ""
#define COLOR_BRIGHT  ""
#define COLOR_FAINT   ""

#define COLOR_BLACK   ""
#define COLOR_RED     ""
#define COLOR_GREEN   ""
#define COLOR_YELLOW  ""
#define COLOR_BLUE    ""
#define COLOR_MAGENTA ""
#define COLOR_CYAN    ""
#define COLOR_WHITE   ""
#endif

#define js_log() (ngx_http_js_module_log != NULL ? ngx_http_js_module_log : ngx_cycle->log)

#if (NGX_DEBUG)

#define ngx_assert(expr) \
if (!(expr)) \
{ \
	ngx_log_error(NGX_LOG_EMERG, ngx_cycle->log, 0, "Assertion failed: (%s), function %s, file %s, line %d.", #expr, __FUNCTION__, __FILE__, __LINE__); \
	ngx_debug_point(); \
}

#define LOG(mess, args...) { fprintf(stderr, mess, ##args); fprintf(stderr, " at %s:%d\n", __FILE__, __LINE__); }
#define LOG2(mess, args...) { fprintf(stderr, mess, ##args); fprintf(stderr, " at %s:%d\n", __FILE__, __LINE__); }

// #define TRACE() { fprintf(stderr, "%s() at %s:%d\n", __FUNCTION__, __FILE__, __LINE__); }
// #define TRACE() { fprintf(stderr, "%s()\n", __FUNCTION__); }
#define TRACE() ngx_log_debug1(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0, COLOR_CYAN "%s" COLOR_CLEAR "()", __FUNCTION__);
#define TRACE_S(s) ngx_log_debug1(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0, COLOR_CYAN "%s" COLOR_CLEAR "(%s)", __FUNCTION__, s);
#define TRACE_STATIC_GETTER() ngx_log_debug2(NGX_LOG_DEBUG_HTTP, js_log(), 0, COLOR_CYAN "get " JS_CLASS_NAME ".%s" COLOR_CLEAR, __FUNCTION__ + 7);
#define TRACE_REQUEST(func) ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "%s(r=%p)", func, r);
#define TRACE_REQUEST_METHOD() ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, COLOR_CYAN "Request#%s" COLOR_CLEAR "(r=%p)", __FUNCTION__ + 7, r);
#define TRACE_REQUEST_GETTER() ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, COLOR_CYAN "get Request#%s" COLOR_CLEAR ", r=%p", __FUNCTION__ + 7, r);
#define TRACE_REQUEST_SETTER() ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, COLOR_CYAN "set Request#%s" COLOR_CLEAR ", r=%p", __FUNCTION__ + 7, r);

#define GET_PRIVATE(private) \
ngx_assert(cx); \
ngx_assert(self); \
if ( (private = JS_GetInstancePrivate(cx, self, private_class, NULL)) == NULL ) \
{ \
	JS_ReportError(cx, "wrapper object has NULL private pointer at %s\n%s: %u", __FUNCTION__, __FILE__, __LINE__); \
	return JS_FALSE; \
}


#else /* NO NGX_DEBUG */

#define ngx_assert(ignore)

#define LOG(mess, args...)
#define LOG2(mess, args...)

#define TRACE()
#define TRACE_S(s)
#define TRACE_STATIC_GETTER()
#define TRACE_REQUEST(func)
#define TRACE_REQUEST_METHOD()
#define TRACE_REQUEST_GETTER()
#define TRACE_REQUEST_SETTER()


#define GET_PRIVATE(private) \
if ( (private = JS_GetInstancePrivate(cx, self, private_class, NULL)) == NULL ) \
{ \
	JS_ReportError(cx, "sorry, the native request had been deleted (see http://wiki.github.com/kung-fu-tzu/ngx_http_js_module/garbagecollecting)"); \
	return JS_FALSE; \
}


#endif


// Just like the throw keyword :)
#define THROW(mess, args...) { JS_ReportError(cx, mess, ##args); return JS_FALSE; }

// Enshure wrapper
// #define E(expr, mess, args...) if (!(expr)) { LOG(#expr); THROW(mess, ##args); return JS_FALSE; }
#define E(expr, mess, args...) if (!(expr)) { THROW(mess, ##args); return JS_FALSE; }



#define NCASE_COMPARE(ngxstring, cstring) ((ngxstring).len == sizeof(cstring) - 1 \
	&& ngx_strncasecmp((ngxstring).data, (u_char *)(cstring), sizeof(cstring) - 1) == 0)

#define CASE_COMPARE(ngxstring, cstring) ((ngxstring).len == sizeof(cstring) - 1 \
	&& ngx_strcasecmp((ngxstring).data, (u_char *)(cstring), sizeof(cstring) - 1) == 0)

#define DATA_LEN_to_JS_STRING_to_JSVAL(cx, data, len, v) \
{ \
	JSString  *value; \
	value = JS_NewStringCopyN(cx, (char *) (data), (len)); \
	if (value == NULL) \
	{ \
		return JS_FALSE; \
	} \
	v = STRING_TO_JSVAL(value); \
}

#define NGX_STRING_to_JS_STRING_to_JSVAL(cx, str, v) DATA_LEN_to_JS_STRING_to_JSVAL(cx, (str).data, (str).len, v);


#endif

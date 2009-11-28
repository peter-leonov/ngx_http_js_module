#ifndef _NGX_HTTP_JS_MACROSES_INCLUDED_
#define _NGX_HTTP_JS_MACROSES_INCLUDED_

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
#define TRACE()
#define TRACE_REQUEST(func) ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "%s(r=%p)", func, r);
#define TRACE_REQUEST_METHOD() ngx_log_debug5(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "Request#%s(r=%p, argc=%d)", __FUNCTION__ + 7, r, argc);

#define GET_PRIVATE(private) \
ngx_assert(cx); \
ngx_assert(this); \
if ( (private = JS_GetInstancePrivate(cx, this, private_class, NULL)) == NULL ) \
{ \
	JS_ReportError(cx, "wrapper object has NULL private pointer at %s\n%s: %u", __FUNCTION__, __FILE__, __LINE__); \
	return JS_FALSE; \
}


#else /* NO NGX_DEBUG */

#define ngx_assert(ignore)

#define LOG(mess, args...)
#define LOG2(mess, args...)

#define TRACE()
#define TRACE_REQUEST(func)
#define TRACE_REQUEST_METHOD()


#define GET_PRIVATE(private) \
if ( (private = JS_GetInstancePrivate(cx, this, private_class, NULL)) == NULL ) \
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


#endif
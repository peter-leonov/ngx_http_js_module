
#ifndef _NGX_HTTP_JS_MACROSES_INCLUDED_
#define _NGX_HTTP_JS_MACROSES_INCLUDED_


#define LOG(mess, args...) { fprintf(stderr, mess, ##args); fprintf(stderr, " at %s:%d\n", __FILE__, __LINE__); }
// #define LOG(mess, args...)

// #define LOG2(mess, args...) { fprintf(stderr, mess, ##args); fprintf(stderr, " at %s:%d\n", __FILE__, __LINE__); }
#define LOG2(mess, args...)

// #define TRACE() { fprintf(stderr, "%s() at %s:%d\n", __FUNCTION__, __FILE__, __LINE__); }
// #define TRACE() { fprintf(stderr, "%s()\n", __FUNCTION__); }
#define TRACE()

#define GET_PRIVATE(args...) \
assert(cx); \
assert(this); \
if ( (r = JS_GetPrivate(cx, this)) == NULL ) \
{ \
	JS_ReportError(cx, "wrapper object has NULL private pointer in %s\n%s: %u", __FUNCTION__, __FILE__, __LINE__); \
	return JS_FALSE; \
}

// Just like the throw keyword :)
#define THROW(mess, args...) { JS_ReportError(cx, mess, ##args); return JS_FALSE; }

// Enshure wrapper
#define E(expr, mess, args...) if (!(expr)) { LOG(#expr); THROW(mess, ##args); return JS_FALSE; }

#define DO_THROW(mess, args...) (throw_error = 1); \
	if (throw_error) { THROW(mess, ##args); }



#define NCASE_COMPARE(ngxstring, cstring) ((ngxstring).len == sizeof(cstring) - 1 \
	&& ngx_strncasecmp((ngxstring).data, (u_char *)(cstring), sizeof(cstring) - 1) == 0)

#define CASE_COMPARE(ngxstring, cstring) ((ngxstring).len == sizeof(cstring) - 1 \
	&& ngx_strcasecmp((ngxstring).data, (u_char *)(cstring), sizeof(cstring) - 1) == 0)

static int throw_error = 0;
// to avoid "not used" warning
static void throw_error_stub() { throw_error = 0; }

#endif

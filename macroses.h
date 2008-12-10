
#ifndef _NGX_HTTP_JS_MACROSES_INCLUDED_
#define _NGX_HTTP_JS_MACROSES_INCLUDED_


#define LOG(mess, args...) fprintf(stderr, mess, ##args); fprintf(stderr, " at %s:%d\n", __FILE__, __LINE__)
// #define LOG2(mess, args...) fprintf(stderr, mess, ##args); fprintf(stderr, " at %s:%d\n", __FILE__, __LINE__)
// #define LOG(mess, args...)
#define LOG2(mess, args...)

#define GET_PRIVATE() \
if ( (r = JS_GetPrivate(cx, this)) == NULL ) \
{ JS_ReportError(cx, "trying to use wrapper object with NULL private pointer"); return JS_FALSE; }

// Enshure wrapper
#define E(expr, mess, args...) \
if (!(expr)) { JS_ReportError(cx, mess, ##args); LOG(#expr); return JS_FALSE; }


#endif

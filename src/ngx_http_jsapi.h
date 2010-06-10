#ifndef _NGX_HTTP_JSAPI_H_INCLUDED_
#define _NGX_HTTP_JSAPI_H_INCLUDED_

#ifndef XP_UNIX
#define XP_UNIX
#endif

#if (NGX_HTTP_JS_MOZJS_HEADERS)
#include <mozjs/jsapi.h>
#else
#include <js/jsapi.h>
#endif

#endif /* _NGX_HTTP_JSAPI_H_INCLUDED_ */

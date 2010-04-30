#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#include <js/jsapi.h>

#include "classes/File.h"

#include "../macroses.h"

#define TRACE_METHOD() \
	ngx_log_debug2(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0, COLOR_CYAN "File#%s" COLOR_CLEAR "(fd=%p)", __FUNCTION__ + 7, fd);
#define TRACE_STATIC_METHOD() \
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0, COLOR_CYAN "File.%s" COLOR_CLEAR "()", __FUNCTION__ + 7);

// according to http://nginx.org/pipermail/nginx-devel/2010-April/000200.html
#define FD_TO_PTR(fd)  ((void *) (uintptr_t) fd)
#define PTR_TO_FD(p)  ((ngx_fd_t) (uintptr_t) p)


JSObject *ngx_http_js__nginx_file__prototype;
JSClass ngx_http_js__nginx_file__class;
static JSClass *private_class = &ngx_http_js__nginx_file__class;

JSObject *
ngx_http_js__nginx_file__wrap(JSContext *cx, ngx_fd_t fd)
{
	JSObject     *file;
	
	TRACE();
	
	file = JS_NewObject(cx, &ngx_http_js__nginx_file__class, ngx_http_js__nginx_file__prototype, NULL);
	if (!file)
	{
		ngx_log_error(NGX_LOG_CRIT, ngx_cycle->log, 0, "could not create a wrapper object for fd (%l)", (long) fd);
		return NULL;
	}
	
	JS_SetPrivate(cx, file, FD_TO_PTR(fd));
	
	return file;
}

static JSBool
method_open(JSContext *cx, JSObject *self, uintN argc, jsval *argv, jsval *rval)
{
	ngx_fd_t         fd;
	JSString        *jss_name;
	JSObject        *file;
	const char      *name;
	
	TRACE_STATIC_METHOD();
	
	E(argc == 1, "Nginx.File#rename takes 1 mandatory argument: name:String");
	
	
	// converting smth. to a string is a very common and rather simple operation,
	// so on failure it's very likely we have gone out of memory
	
	jss_name = JS_ValueToString(cx, argv[0]);
	if (jss_name == NULL)
	{
		JS_ReportOutOfMemory(cx);
		return JS_FALSE;
	}
	
	name = JS_GetStringBytes(jss_name);
	if (name[0] == '\0')
	{
		JS_ReportOutOfMemory(cx);
		return JS_FALSE;
	}
	
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0, "ngx_open_file(\"%s\")", name);
	fd = ngx_open_file(name, NGX_FILE_RDWR, NGX_FILE_CREATE_OR_OPEN, NGX_FILE_DEFAULT_ACCESS);
	
	if (fd == NGX_INVALID_FILE)
	{
		*rval = JSVAL_VOID;
		return JS_TRUE;
	}
	
	file = ngx_http_js__nginx_file__wrap(cx, fd);
	if (file == NULL)
	{
		JS_ReportOutOfMemory(cx);
		return JS_FALSE;
	}
	
	*rval = OBJECT_TO_JSVAL(file);
	return JS_TRUE;
}

static JSBool
method_rename(JSContext *cx, JSObject *self, uintN argc, jsval *argv, jsval *rval)
{
	JSString        *jss_from, *jss_to;
	const char      *from, *to;
	
	TRACE_STATIC_METHOD();
	
	E(argc == 2, "Nginx.File#rename takes 2 mandatory arguments: from:String and to:String");
	
	
	// converting smth. to a string is a very common and rather simple operation,
	// so on failure it's very likely we have gone out of memory
	
	jss_from = JS_ValueToString(cx, argv[0]);
	if (jss_from == NULL)
	{
		JS_ReportOutOfMemory(cx);
		return JS_FALSE;
	}
	
	from = JS_GetStringBytes(jss_from);
	if (from[0] == '\0')
	{
		JS_ReportOutOfMemory(cx);
		return JS_FALSE;
	}
	
	
	jss_to = JS_ValueToString(cx, argv[1]);
	if (jss_to == NULL)
	{
		JS_ReportOutOfMemory(cx);
		return JS_FALSE;
	}
	
	to = JS_GetStringBytes(jss_to);
	if (to[0] == '\0')
	{
		JS_ReportOutOfMemory(cx);
		return JS_FALSE;
	}
	
	ngx_log_debug2(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0, "ngx_rename_file(\"%s\", \"%s\")", from, to);
	
	*rval = INT_TO_JSVAL(ngx_rename_file(from, to));
	
	return JS_TRUE;
}

static JSBool
method_remove(JSContext *cx, JSObject *self, uintN argc, jsval *argv, jsval *rval)
{
	JSString        *jss_name;
	const char      *name;
	
	TRACE_STATIC_METHOD();
	
	E(argc == 1, "Nginx.File#delete takes 1 mandatory argument: name:String");
	
	
	// converting smth. to a string is a very common and rather simple operation,
	// so on failure it's very likely we have gone out of memory
	
	jss_name = JS_ValueToString(cx, argv[0]);
	if (jss_name == NULL)
	{
		JS_ReportOutOfMemory(cx);
		return JS_FALSE;
	}
	
	name = JS_GetStringBytes(jss_name);
	if (name[0] == '\0')
	{
		JS_ReportOutOfMemory(cx);
		return JS_FALSE;
	}
	
	
	ngx_log_debug2(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0, "ngx_delete_file(\"%s\")", name);
	*rval = INT_TO_JSVAL(ngx_delete_file(name));
	
	return JS_TRUE;
}

static JSBool
method_write(JSContext *cx, JSObject *self, uintN argc, jsval *argv, jsval *rval)
{
	JSString        *jss_str;
	char            *str;
	ngx_fd_t         fd;
	size_t           len;
	void            *p;
	
	GET_PRIVATE(p);
	fd = PTR_TO_FD(p);
	TRACE_METHOD();
	
	E(argc == 1, "Nginx.File#write takes 1 mandatory argument: data:String");
	
	
	// converting smth. to a string is a very common and rather simple operation,
	// so on failure it's very likely we have gone out of memory
	
	jss_str = JS_ValueToString(cx, argv[0]);
	if (jss_str == NULL)
	{
		JS_ReportOutOfMemory(cx);
		return JS_FALSE;
	}
	
	str = JS_GetStringBytes(jss_str);
	if (str[0] == '\0')
	{
		JS_ReportOutOfMemory(cx);
		return JS_FALSE;
	}
	
	len = ngx_strlen(str);
	
	ngx_log_debug2(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0, "ngx_write_fd(fd=%d, len=%d)", fd, len);
	*rval = ngx_write_fd(fd, str, len) == (ssize_t) len ? JSVAL_TRUE : JSVAL_FALSE;
	
	return JS_TRUE;
}

static JSBool
method_read(JSContext *cx, JSObject *self, uintN argc, jsval *argv, jsval *rval)
{
	ngx_fd_t         fd;
	void            *p;
	
	GET_PRIVATE(p);
	fd = PTR_TO_FD(p);
	TRACE_METHOD();
	
	E(argc == 1, "Nginx.File#read takes 1 mandatory argument: length:Number");
	
	
	return JS_TRUE;
}


static JSBool
constructor(JSContext *cx, JSObject *self, uintN argc, jsval *argv, jsval *rval)
{
	TRACE();
	return JS_TRUE;
}

static JSBool
static_getProperty(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	TRACE();
	
	// LOG("Nginx property id = %d\n", JSVAL_TO_INT(id));
	if (JSVAL_IS_INT(id))
	{
		switch (JSVAL_TO_INT(id))
		{
			case 1:  *vp = INT_TO_JSVAL(NGX_INVALID_FILE); break;
			case 2:  *vp = INT_TO_JSVAL(NGX_FILE_ERROR); break;
			
#ifdef NGX_HAVE_CASELESS_FILESYSTEM
			case 3:  *vp = JSVAL_TRUE; break;
#else
			case 3:  *vp = JSVAL_FALSE; break;
#endif
			
			case 4:  *vp = INT_TO_JSVAL(NGX_FILE_RDONLY); break;
			case 5:  *vp = INT_TO_JSVAL(NGX_FILE_WRONLY); break;
			case 6:  *vp = INT_TO_JSVAL(NGX_FILE_RDWR); break;
			case 7:  *vp = INT_TO_JSVAL(NGX_FILE_CREATE_OR_OPEN); break;
			case 8:  *vp = INT_TO_JSVAL(NGX_FILE_OPEN); break;
			case 9:  *vp = INT_TO_JSVAL(NGX_FILE_TRUNCATE); break;
			case 10: *vp = INT_TO_JSVAL(NGX_FILE_APPEND); break;
			case 11: *vp = INT_TO_JSVAL(NGX_FILE_NONBLOCK); break;
			case 12: *vp = INT_TO_JSVAL(NGX_FILE_DEFAULT_ACCESS); break;
			case 13: *vp = INT_TO_JSVAL(NGX_FILE_OWNER_ACCESS); break;
		}
	}
	
	return JS_TRUE;
}

static JSFunctionSpec funcs[] =
{
	{"write",           method_write,               1, 0, 0},
	{"read",            method_read,                1, 0, 0},
	{0, NULL, 0, 0, 0}
};

static JSPropertySpec props[] =
{
	{0, 0, 0, NULL, NULL}
};


static JSFunctionSpec static_funcs[] =
{
	{"rename",           method_rename,               2, 0, 0},
	{"open",             method_open,                 1, 0, 0},
	{"remove",           method_remove,               1, 0, 0},
	{0, NULL, 0, 0, 0}
};

static JSPropertySpec static_props[] =
{
	{"INVALID",                  1,  JSPROP_READONLY, static_getProperty, NULL},
	{"ERROR",                    2,  JSPROP_READONLY, static_getProperty, NULL},
	
	{"HAVE_CASELESS_FILESYSTEM", 3,  JSPROP_READONLY, static_getProperty, NULL},
	
	{"RDONLY",                   4,  JSPROP_READONLY, static_getProperty, NULL},
	{"WRONLY",                   5,  JSPROP_READONLY, static_getProperty, NULL},
	{"RDWR",                     6,  JSPROP_READONLY, static_getProperty, NULL},
	{"CREATE_OR_OPEN",           7,  JSPROP_READONLY, static_getProperty, NULL},
	{"OPEN",                     8,  JSPROP_READONLY, static_getProperty, NULL},
	{"TRUNCATE",                 9,  JSPROP_READONLY, static_getProperty, NULL},
	{"APPEND",                   10, JSPROP_READONLY, static_getProperty, NULL},
	{"NONBLOCK",                 11, JSPROP_READONLY, static_getProperty, NULL},
	{"DEFAULT_ACCESS",           12, JSPROP_READONLY, static_getProperty, NULL},
	{"OWNER_ACCESS",             13, JSPROP_READONLY, static_getProperty, NULL},
	  
	{0, 0, 0, NULL, NULL}
};

JSClass ngx_http_js__nginx_file__class =
{
	"File",
	0,
	JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

JSBool
ngx_http_js__nginx_file__init(JSContext *cx, JSObject *global)
{
	JSObject    *nginxobj;
	jsval        vp;
	
	E(JS_GetProperty(cx, global, "Nginx", &vp), "global.Nginx is undefined");
	nginxobj = JSVAL_TO_OBJECT(vp);
	
	ngx_http_js__nginx_file__prototype = JS_InitClass(cx, nginxobj, NULL, &ngx_http_js__nginx_file__class,  constructor, 0,
		props, funcs, static_props, static_funcs);
	E(ngx_http_js__nginx_file__prototype, "Can`t JS_InitClass(Nginx.File)");
	
	return JS_TRUE;
}

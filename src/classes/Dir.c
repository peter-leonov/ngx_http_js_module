#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#include <ngx_http_jsapi.h>

#include <ngx_http_js_module.h>
#include <nginx_js_glue.h>
#include <classes/Dir.h>

#include <nginx_js_macroses.h>

#define TRACE_METHOD() \
	ngx_log_debug2(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0, COLOR_CYAN "Dir#%s" COLOR_CLEAR "(fd=%p)", __FUNCTION__ + 7, fd);
#define TRACE_STATIC_METHOD() \
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0, COLOR_CYAN "Dir.%s" COLOR_CLEAR "()", __FUNCTION__ + 7);


JSObject *ngx_http_js__nginx_dir__prototype;
JSClass ngx_http_js__nginx_dir__class;
// static JSClass *private_class = &ngx_http_js__nginx_dir__class;

typedef struct
{
	jsval     onfile, onenter, onleave, onspecial;
} walkTree_ctx;



static JSBool
method_create(JSContext *cx, uintN argc, jsval *vp)
{
	ngx_uint_t       access;
	JSString        *jss_path;
	jsdouble         dp;
	char             path[NGX_MAX_PATH];
	size_t           len;
	
	TRACE_STATIC_METHOD();
	
	E(argc == 2, "Nginx.Dir#create takes 2 mandatory arguments: path:String and access:Number");
	
	
	// converting smth. to a string is a very common and rather simple operation,
	// so on failure it's very likely we have gone out of memory
	
	jss_path = JS_ValueToString(cx, JS_ARGV(cx, vp)[0]);
	if (jss_path == NULL)
	{
		return JS_FALSE;
	}
	
	len = JS_EncodeStringToBuffer(jss_path, path, NGX_MAX_PATH - 1);
	if (len == (size_t) -1 || len > NGX_MAX_PATH - 1)
	{
		return JS_FALSE;
	}
	
	
	if (!JS_ValueToNumber(cx, JS_ARGV(cx, vp)[1], &dp))
	{
		// forward exception if any
		return JS_FALSE;
	}
	
	access = dp;
	
	ngx_log_debug2(NGX_LOG_DEBUG_HTTP, js_log(), 0, "ngx_create_dir(\"%s\", %d)", path, access);
	JS_SET_RVAL(cx, vp, INT_TO_JSVAL(ngx_create_dir(path, access)));
	
	return JS_TRUE;
}


static JSBool
method_createPath(JSContext *cx, uintN argc, jsval *vp)
{
	ngx_uint_t       access;
	JSString        *jss_path;
	jsdouble         dp;
	char             path[NGX_MAX_PATH];
	size_t           len;
	
	TRACE_STATIC_METHOD();
	
	E(argc == 2, "Nginx.Dir#createPath takes 2 mandatory arguments: path:String and access:Number");
	
	jss_path = JS_ValueToString(cx, JS_ARGV(cx, vp)[0]);
	if (jss_path == NULL)
	{
		return JS_FALSE;
	}
	
	len = JS_EncodeStringToBuffer(jss_path, path, NGX_MAX_PATH - 1);
	if (len == (size_t) -1 || len > NGX_MAX_PATH - 1)
	{
		return JS_FALSE;
	}
	
	if (!JS_ValueToNumber(cx, JS_ARGV(cx, vp)[1], &dp))
	{
		return JS_FALSE;
	}
	
	access = dp;
	
	ngx_log_debug2(NGX_LOG_DEBUG_HTTP, js_log(), 0, "ngx_create_full_path(\"%s\", %d)", (u_char *) path, access);
	// ngx_create_full_path() needs a writable copy of the path
	JS_SET_RVAL(cx, vp, INT_TO_JSVAL(ngx_create_full_path((u_char *) path, access)));
	
	return JS_TRUE;
}


static ngx_int_t
tree_noop(ngx_tree_ctx_t *ctx, ngx_str_t *path)
{
	return NGX_OK;
}


static ngx_int_t
tree_delete_file(ngx_tree_ctx_t *ctx, ngx_str_t *path)
{
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, ctx->log, 0, "ngx_delete_file(\"%s\")", path->data);
	
	if (ngx_delete_file(path->data) == NGX_FILE_ERROR)
	{
		ngx_log_error(NGX_LOG_CRIT, ctx->log, ngx_errno, ngx_delete_file_n " \"%s\" failed", path->data);
	}
	
	return NGX_OK;
}


static ngx_int_t
tree_delete_dir(ngx_tree_ctx_t *ctx, ngx_str_t *path)
{
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, ctx->log, 0, "ngx_delete_dir(\"%s\")", path->data);
	
	if (ngx_delete_dir(path->data) == NGX_FILE_ERROR)
	{
		ngx_log_error(NGX_LOG_CRIT, ctx->log, ngx_errno, ngx_delete_dir_n " \"%s\" failed", path->data);
	}
	
	return NGX_OK;
}


static JSBool
method_removeTree(JSContext *cx, uintN argc, jsval *vp)
{
	ngx_tree_ctx_t   tree;
	ngx_int_t        rc;
	ngx_str_t        path_ns;
	JSString        *jss_path;
	u_char           path[NGX_MAX_PATH];
	size_t           len;
	
	TRACE_STATIC_METHOD();
	
	E(argc == 1, "Nginx.Dir#removeTree takes 1 mandatory argument: path:String");
	
	jss_path = JS_ValueToString(cx, JS_ARGV(cx, vp)[0]);
	if (jss_path == NULL)
	{
		return JS_FALSE;
	}
	
	len = JS_EncodeStringToBuffer(jss_path, (char *) path, NGX_MAX_PATH - 1);
	if (len == (size_t) -1 || len > NGX_MAX_PATH - 1)
	{
		return JS_FALSE;
	}
	
	path_ns.len = ngx_strlen(path);
	path_ns.data = path;
	
	tree.init_handler = NULL;
	tree.file_handler = tree_delete_file;
	tree.pre_tree_handler = tree_noop;
	tree.post_tree_handler = tree_delete_dir;
	tree.spec_handler = tree_delete_file;
	tree.data = NULL;
	tree.alloc = 0;
	tree.log = js_log();
	
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, js_log(), 0, "ngx_walk_tree(\"%s\")", path);
	rc = ngx_walk_tree(&tree, &path_ns);
	if (rc == NGX_ABORT)
	{
		JS_ReportOutOfMemory(cx);
		return JS_FALSE;
	}
	
	if (rc != NGX_OK)
	{
		JS_SET_RVAL(cx, vp, INT_TO_JSVAL(rc));
		return JS_TRUE;
	}
	
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, js_log(), 0, "ngx_delete_dir(\"%s\")", path);
	JS_SET_RVAL(cx, vp, INT_TO_JSVAL(ngx_delete_dir(path)));
	
	return JS_TRUE;
}


static ngx_int_t
walkTree_file_handler(ngx_tree_ctx_t *ctx, ngx_str_t *path)
{
	JSString         *path_jss;
	JSContext        *cx;
	jsval             args[4], rval;
	walkTree_ctx     *wt_ctx;
	
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, ctx->log, 0, "walkTree_file_handler(\"%s\")", path->data);
	
	wt_ctx = ctx->data;
	cx = js_cx;
	
	// bake the file path
	path_jss = JS_NewStringCopyN(cx, (char *) (path->data), path->len);
	if (path_jss == NULL)
	{
		return NGX_ABORT;
	}
	args[0] = STRING_TO_JSVAL(path_jss);
	
	// bake the file size
	if (!JS_NewNumberValue(cx, ctx->size, &args[1]))
	{
		return NGX_ABORT;
	}
	
	// bake the file access
	args[2] = INT_TO_JSVAL(ctx->access);
	
	// bake the file mtime
	if (!JS_NewNumberValue(cx, ctx->mtime, &args[3]))
	{
		return NGX_ABORT;
	}
	
	// call the handler and hope for best ;)
	if (!JS_CallFunctionValue(cx, js_global, wt_ctx->onfile, 4, args, &rval))
	{
		// TODO: somehow check the exception type and return NGX_ABORT only on OOM one
		return NGX_ABORT;
	}
	
	if (JSVAL_IS_VOID(rval))
	{
		return NGX_OK;
	}
	
	return JSVAL_TO_INT(rval);
}


static ngx_int_t
walkTree_pre_tree_handler(ngx_tree_ctx_t *ctx, ngx_str_t *path)
{
	JSString         *path_jss;
	JSContext        *cx;
	jsval             args[3], rval;
	walkTree_ctx     *wt_ctx;
	
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, ctx->log, 0, "walkTree_pre_tree_handler(\"%s\")", path->data);
	
	wt_ctx = ctx->data;
	cx = js_cx;
	
	// bake the dir path
	path_jss = JS_NewStringCopyN(cx, (char *) (path->data), path->len);
	if (path_jss == NULL)
	{
		return NGX_ABORT;
	}
	args[0] = STRING_TO_JSVAL(path_jss);
	
	// bake the dir access
	args[1] = INT_TO_JSVAL(ctx->access);
	
	// bake the dir mtime
	if (!JS_NewNumberValue(cx, ctx->mtime, &args[2]))
	{
		return NGX_ABORT;
	}
	
	// call the handler and hope for best ;)
	if (!JS_CallFunctionValue(cx, js_global, wt_ctx->onenter, 3, args, &rval))
	{
		// TODO: somehow check the exception type and return NGX_ABORT only on OOM one
		return NGX_ABORT;
	}
	
	if (JSVAL_IS_VOID(rval))
	{
		return NGX_OK;
	}
	
	return JSVAL_TO_INT(rval);
}


static ngx_int_t
walkTree_post_tree_handler(ngx_tree_ctx_t *ctx, ngx_str_t *path)
{
	JSString         *path_jss;
	JSContext        *cx;
	jsval             args[3], rval;
	walkTree_ctx     *wt_ctx;
	
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, ctx->log, 0, "walkTree_post_tree_handler(\"%s\")", path->data);
	
	wt_ctx = ctx->data;
	cx = js_cx;
	
	// bake the dir path
	path_jss = JS_NewStringCopyN(cx, (char *) (path->data), path->len);
	if (path_jss == NULL)
	{
		return NGX_ABORT;
	}
	args[0] = STRING_TO_JSVAL(path_jss);
	
	// bake the dir access
	args[1] = INT_TO_JSVAL(ctx->access);
	
	// bake the dir mtime
	if (!JS_NewNumberValue(cx, ctx->mtime, &args[2]))
	{
		return NGX_ABORT;
	}
	
	// call the handler and hope for best ;)
	if (!JS_CallFunctionValue(cx, js_global, wt_ctx->onleave, 3, args, &rval))
	{
		// TODO: somehow check the exception type and return NGX_ABORT only on OOM one
		return NGX_ABORT;
	}
	
	if (JSVAL_IS_VOID(rval))
	{
		return NGX_OK;
	}
	
	return JSVAL_TO_INT(rval);
}


static ngx_int_t
walkTree_spec_handler(ngx_tree_ctx_t *ctx, ngx_str_t *path)
{
	JSString         *path_jss;
	JSContext        *cx;
	jsval             args[1], rval;
	walkTree_ctx     *wt_ctx;
	
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, ctx->log, 0, "walkTree_file_handler(\"%s\")", path->data);
	
	wt_ctx = ctx->data;
	cx = js_cx;
	
	// bake the file path
	path_jss = JS_NewStringCopyN(cx, (char *) (path->data), path->len);
	if (path_jss == NULL)
	{
		return NGX_ABORT;
	}
	args[0] = STRING_TO_JSVAL(path_jss);
	
	// call the handler and hope for best ;)
	if (!JS_CallFunctionValue(cx, js_global, wt_ctx->onspecial, 1, args, &rval))
	{
		// TODO: somehow check the exception type and return NGX_ABORT only on OOM one
		return NGX_ABORT;
	}
	
	if (JSVAL_IS_VOID(rval))
	{
		return NGX_OK;
	}
	
	return JSVAL_TO_INT(rval);
}


static JSBool
method_walkTree(JSContext *cx, uintN argc, jsval *vp)
{
	ngx_tree_ctx_t   tree;
	ngx_int_t        rc;
	ngx_str_t        path_ns;
	JSString        *jss_path;
	u_char           path[NGX_MAX_PATH];
	size_t           len;
	walkTree_ctx     ctx;
	
	TRACE_STATIC_METHOD();
	
	E(argc >= 1 && argc <= 5, "Nginx.Dir#walkTree takes 1 mandatory argument: path:String, and four optional callbacks");
	
	jss_path = JS_ValueToString(cx, JS_ARGV(cx, vp)[0]);
	if (jss_path == NULL)
	{
		return JS_FALSE;
	}
	
	len = JS_EncodeStringToBuffer(jss_path, (char *) path, NGX_MAX_PATH - 1);
	if (len == (size_t) -1 || len > NGX_MAX_PATH - 1)
	{
		return JS_FALSE;
	}
	
	path_ns.len = ngx_strlen(path);
	path_ns.data = path;
	
	
	// file callback
	if (argc >= 2 && !JSVAL_IS_NULL(JS_ARGV(cx, vp)[1]))
	{
		if (!JSVAL_IS_OBJECT(JS_ARGV(cx, vp)[1]) || !JS_ObjectIsFunction(cx, JSVAL_TO_OBJECT(JS_ARGV(cx, vp)[1])))
		{
			JS_ReportError(cx, "file callback must be a function or a null");
			return JS_FALSE;
		}
		
		ctx.onfile = JS_ARGV(cx, vp)[1];
		tree.file_handler = walkTree_file_handler;
	}
	else
	{
		tree.file_handler = tree_noop;
	}
	
	// dir enter callback
	if (argc >= 3 && !JSVAL_IS_NULL(JS_ARGV(cx, vp)[2]))
	{
		if (!JSVAL_IS_OBJECT(JS_ARGV(cx, vp)[2]) || !JS_ObjectIsFunction(cx, JSVAL_TO_OBJECT(JS_ARGV(cx, vp)[2])))
		{
			JS_ReportError(cx, "directory enter callback must be a function or a null");
			return JS_FALSE;
		}
		
		ctx.onenter = JS_ARGV(cx, vp)[2];
		tree.pre_tree_handler = walkTree_pre_tree_handler;
	}
	else
	{
		tree.pre_tree_handler = tree_noop;
	}
	
	// dir leave callback
	if (argc >= 4 && !JSVAL_IS_NULL(JS_ARGV(cx, vp)[3]))
	{
		if (!JSVAL_IS_OBJECT(JS_ARGV(cx, vp)[3]) || !JS_ObjectIsFunction(cx, JSVAL_TO_OBJECT(JS_ARGV(cx, vp)[3])))
		{
			JS_ReportError(cx, "directory leave callback must be a function or a null");
			return JS_FALSE;
		}
		
		ctx.onleave = JS_ARGV(cx, vp)[3];
		tree.post_tree_handler = walkTree_post_tree_handler;
	}
	else
	{
		tree.post_tree_handler = tree_noop;
	}
	
	// special entry callback
	if (argc >= 5 && !JSVAL_IS_NULL(JS_ARGV(cx, vp)[4]))
	{
		if (!JSVAL_IS_OBJECT(JS_ARGV(cx, vp)[4]) || !JS_ObjectIsFunction(cx, JSVAL_TO_OBJECT(JS_ARGV(cx, vp)[4])))
		{
			JS_ReportError(cx, "special entry callback must be a function or a null");
			return JS_FALSE;
		}
		
		ctx.onspecial = JS_ARGV(cx, vp)[4];
		tree.spec_handler = walkTree_spec_handler;
	}
	else
	{
		tree.spec_handler = tree_noop;
	}
	
	
	tree.init_handler = NULL;
	tree.data = &ctx;
	tree.alloc = 0;
	tree.log = js_log();
	
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, js_log(), 0, "ngx_walk_tree(\"%s\")", path);
	rc = ngx_walk_tree(&tree, &path_ns);
	if (rc == NGX_ABORT)
	{
		JS_ReportOutOfMemory(cx);
		return JS_FALSE;
	}
	
	JS_SET_RVAL(cx, vp, INT_TO_JSVAL(rc));
	return JS_TRUE;
}


static JSBool
method_remove(JSContext *cx, uintN argc, jsval *vp)
{
	JSString        *jss_name;
	char             name[NGX_MAX_PATH];
	size_t           len;
	
	TRACE_STATIC_METHOD();
	
	E(argc == 1, "Nginx.Dir#remove takes 1 mandatory argument: name:String");
	
	jss_name = JS_ValueToString(cx, JS_ARGV(cx, vp)[0]);
	if (jss_name == NULL)
	{
		return JS_FALSE;
	}
	
	len = JS_EncodeStringToBuffer(jss_name, name, NGX_MAX_PATH - 1);
	if (len == (size_t) -1 || len > NGX_MAX_PATH - 1)
	{
		return JS_FALSE;
	}
	
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, js_log(), 0, "ngx_delete_dir(\"%s\")", name);
	JS_SET_RVAL(cx, vp, INT_TO_JSVAL(ngx_delete_dir(name)));
	
	return JS_TRUE;
}


static JSBool
constructor(JSContext *cx, uintN argc, jsval *vp)
{
	TRACE();
	JS_SetPrivate(cx, JS_THIS_OBJECT(cx, vp), NULL);
	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;
}

static void
finalizer(JSContext *cx, JSObject *self)
{
	TRACE();
	JS_SetPrivate(cx, self, NULL);
}

static JSFunctionSpec funcs[] =
{
	JS_FS_END
};

static JSPropertySpec props[] =
{
	{0, 0, 0, NULL, NULL}
};


static JSFunctionSpec static_funcs[] =
{
	JS_FS("create",             method_create,               2, 0),
	JS_FS("createPath",         method_createPath,           2, 0),
	JS_FS("remove",             method_remove,               2, 0),
	JS_FS("removeTree",         method_removeTree,           1, 0),
	JS_FS("walkTree",           method_walkTree,             1, 0),
	JS_FS_END
};

static JSPropertySpec static_props[] =
{
	{0, 0, 0, NULL, NULL}
};

JSClass ngx_http_js__nginx_dir__class =
{
	"Dir",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, finalizer,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

JSBool
ngx_http_js__nginx_dir__init(JSContext *cx, JSObject *global)
{
	static int  inited = 0;
	JSObject    *nginxobj;
	jsval        vp;
	
	if (inited)
	{
		JS_ReportError(cx, "Nginx.Dir is already inited");
		return JS_FALSE;
	}
	
	E(JS_GetProperty(cx, global, "Nginx", &vp), "global.Nginx is undefined");
	nginxobj = JSVAL_TO_OBJECT(vp);
	
	ngx_http_js__nginx_dir__prototype = JS_InitClass(cx, nginxobj, NULL, &ngx_http_js__nginx_dir__class,  constructor, 0,
		props, funcs, static_props, static_funcs);
	E(ngx_http_js__nginx_dir__prototype, "Can`t JS_InitClass(Nginx.Dir)");
	
	inited = 1;
	
	return JS_TRUE;
}

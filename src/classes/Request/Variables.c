#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#include <ngx_http_jsapi.h>

#include <ngx_http_js_module.h>
#include <nginx_js_glue.h>
#include <strings_util.h>
#include <classes/Request.h>

#include <nginx_js_macroses.h>


JSObject *ngx_http_js__nginx_variables__prototype;
JSClass ngx_http_js__nginx_variables__class;
static JSClass* private_class = &ngx_http_js__nginx_variables__class;


static ngx_int_t
set_variable(ngx_http_request_t *r, ngx_str_t *name, ngx_uint_t key, ngx_http_variable_value_t *value);


JSObject *
ngx_http_js__nginx_variables__wrap(JSContext *cx, ngx_http_request_t *r)
{
	JSObject                  *variables;
	ngx_http_js_ctx_t         *ctx;
	
	TRACE();
	
	ctx = ngx_http_get_module_ctx(r, ngx_http_js_module);
	if (ctx == NULL)
	{
		return NULL;
	}
	
	if (ctx->js_variables != NULL)
	{
		return ctx->js_variables;
	}
	
	variables = JS_NewObject(cx, &ngx_http_js__nginx_variables__class, ngx_http_js__nginx_variables__prototype, NULL);
	if (variables == NULL)
	{
		JS_ReportOutOfMemory(cx);
		return NULL;
	}
	
	if (!JS_SetReservedSlot(cx, ctx->js_request, NGX_JS_REQUEST_SLOT__VARIABLES, OBJECT_TO_JSVAL(variables)))
	{
		JS_ReportError(cx, "can't set slot NGX_JS_REQUEST_SLOT__VARIABLES(%d)", NGX_JS_REQUEST_SLOT__VARIABLES);
		return NULL;
	}
	
	JS_SetPrivate(cx, variables, r);
	
	ctx->js_variables = variables;
	
	return variables;
}


void
ngx_http_js__nginx_variables__cleanup(ngx_http_js_ctx_t *ctx, ngx_http_request_t *r, JSContext *cx)
{
	ngx_assert(ctx);
	
	if (ctx->js_variables == NULL)
	{
		return;
	}
	
	JS_SetPrivate(cx, ctx->js_variables, NULL);
	ctx->js_variables = NULL;
}

static JSBool
constructor(JSContext *cx, uintN argc, jsval *vp)
{
	TRACE();
	JS_ReportError(cx, "Nginx.Request.Variables instance may be constucted with request#variables only");
	return JS_FALSE;
}


static JSBool
getProperty(JSContext *cx, JSObject *self, jsid id, jsval *vp)
{
	ngx_http_request_t         *r;
	ngx_str_t                   var;
	ngx_uint_t                  hash;
	ngx_http_variable_value_t  *vv;
	jsval                       idval;
	JSString                   *key_jss;
	
	TRACE();
	GET_PRIVATE(r);
	
	if (!JS_IdToValue(cx, id, &idval))
	{
		return JS_FALSE;
	}
	
	key_jss = JS_ValueToString(cx, idval);
	if (key_jss == NULL)
	{
		return JS_FALSE;
	}
	
	// we have to create the string on the request pool
	// because the variable name may be cached just after
	// the first seach for it
	if (!js_str2ngx_str(cx, key_jss, r->pool, &var))
	{
		return JS_FALSE;
	}
	
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "variables[\"%s\"]", var.data);
	
	// lowercase in-place
	hash = ngx_hash_strlow(var.data, var.data, var.len);
	
	// ngx_http_get_variable() calls v->get_handler() and
	// checks for *special* variables kinda $http_*, $cookie_* and others,
	// so we can rely on it – all job will done for us
#if defined(nginx_version) && (nginx_version >= 8036)
	vv = ngx_http_get_variable(r, &var, hash);
#else
	vv = ngx_http_get_variable(r, &var, hash, 1);
#endif
	if (vv == NULL)
	{
		JS_ReportOutOfMemory(cx);
		return JS_FALSE;
	}
	
	if (vv->not_found)
	{
		*vp = JSVAL_VOID;
		return JS_TRUE;
	}
	
	NGX_STRING_to_JS_STRING_to_JSVAL(cx, *vv, *vp);
	
	return JS_TRUE;
}

static JSBool
setProperty(JSContext *cx, JSObject *self, jsid id, JSBool strict, jsval *vp)
{
	ngx_http_request_t         *r;
	ngx_str_t                   var, val;
	ngx_uint_t                  hash;
	ngx_http_variable_value_t  *vv;
	ngx_int_t                   rc;
	jsval                       idval;
	JSString                   *key_jss, *value_jss;
	size_t                      len;
	
	TRACE();
	GET_PRIVATE(r);
	
	if (!JS_IdToValue(cx, id, &idval))
	{
		return JS_FALSE;
	}
	
	key_jss = JS_ValueToString(cx, idval);
	if (key_jss == NULL)
	{
		return JS_FALSE;
	}
	
	// we have to create the string on the request pool
	// because the variable name may be cached just after
	// the first seach for the var
	if (!js_str2ngx_str(cx, key_jss, r->pool, &var))
	{
		return JS_FALSE;
	}
	
	
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "variables[\"%s\"]", var.data);
	
	// lowercase in-place
	hash = ngx_hash_strlow(var.data, var.data, len);
	
	value_jss = JS_ValueToString(cx, *vp);
	if (value_jss == NULL)
	{
		return JS_FALSE;
	}
	
	if (!js_str2ngx_str(cx, value_jss, r->pool, &val))
	{
		return JS_FALSE;
	}
	
	vv = ngx_pnalloc(r->pool, sizeof(ngx_http_variable_value_t));
	if (vv == NULL)
	{
		JS_ReportOutOfMemory(cx);
		return JS_FALSE;
	}
	
	vv->len = val.len;
	vv->data = val.data;
	
	rc = set_variable(r, &var, hash, vv);
	if (rc == NGX_DECLINED)
	{
		JS_ReportError(cx, "can't find variable \"%s\"", var.data);
		return JS_FALSE;
	}
	
	return JS_TRUE;
}


static JSPropertySpec ngx_http_js__nginx_variables__props[] =
{
	// {"length",                 1,          JSPROP_READONLY,   NULL, NULL},
	{0, 0, 0, NULL, NULL}
};


static JSFunctionSpec ngx_http_js__nginx_variables__funcs[] =
{
	// JS_FS("empty",       method_empty,          0, 0, 0),
	JS_FS_END
};

JSClass ngx_http_js__nginx_variables__class =
{
	"Variables",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub, getProperty, setProperty,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

JSBool
ngx_http_js__nginx_variables__init(JSContext *cx, JSObject *global)
{
	JSObject    *nginxobj;
	jsval        vp;
	
	E(JS_GetProperty(cx, global, "Nginx", &vp), "global.Nginx is undefined");
	nginxobj = JSVAL_TO_OBJECT(vp);
	
	ngx_http_js__nginx_variables__prototype = JS_InitClass(cx, nginxobj, NULL, &ngx_http_js__nginx_variables__class,  constructor, 0,
		ngx_http_js__nginx_variables__props, ngx_http_js__nginx_variables__funcs,  NULL, NULL);
	E(ngx_http_js__nginx_variables__prototype, "Can`t JS_InitClass(Nginx.Request.Variables)");
	
	return JS_TRUE;
}


static ngx_int_t
set_variable(ngx_http_request_t *r, ngx_str_t *name, ngx_uint_t key, ngx_http_variable_value_t *value)
{
	ngx_http_variable_t        *v;
	ngx_http_variable_value_t  *vv;
	ngx_http_core_main_conf_t  *cmcf;
	
	cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module);
	
	v = ngx_hash_find(&cmcf->variables_hash, key, name->data, name->len);
	
	if (v == NULL)
	{
		return NGX_DECLINED;
	}
	
	if (v->flags & NGX_HTTP_VAR_INDEXED)
	{
		vv = &r->variables[v->index];
		vv->data = value->data;
		vv->len = value->len;
		vv->valid = 1;
		vv->not_found = 0;
		
		return NGX_OK;
	}
	
	if (v->set_handler == NULL)
	{
		return NGX_ABORT;
	}
	
	v->set_handler(r, value, v->data);
	
	return NGX_OK;
}

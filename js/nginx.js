
if (self.Nginx)
(function(){
	var N = Nginx
	function flat (args) { return Array.prototype.slice.apply(args).join(', ') }
	N.log   = function () { this.logError(this.NGX_LOG_WARN,  flat(arguments)) } // NGX_LOG_DEBUG
	N.info  = function () { this.logError(this.NGX_LOG_INFO,  flat(arguments)) }
	N.warn  = function () { this.logError(this.NGX_LOG_WARN,  flat(arguments)) }
	N.error = function () { this.logError(this.NGX_LOG_ERR,   flat(arguments)) }
	log = function () { N.log.apply(N, arguments) }
})()

load('lib.js')


// try
// {
// Nginx.warn('warn')
// // Nginx.logError(null, 'error')
// Nginx.warn('warn')
// }
// catch (ex) { Nginx.error(ex) }

function processRequest (r)
{
	// log(r.uri, r.method, r.remoteAddr)
	r.sendHttpHeader('text/html; charset=utf-8')
	r.printString("Привет, Петя!" + r.uri + r.method + r.remoteAddr)
	
	return Nginx.NGX_HTTP_OK
}


log("Nginx.js loaded")


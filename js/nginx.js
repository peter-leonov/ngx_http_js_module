
// Nginx wrapers
if (!self.Nginx)
	throw 'global.Nginx is undefined'

;(function(){
	var N = Nginx
	function flat (args) { return Array.prototype.slice.apply(args).join(', ') }
	N.log   = function () { this.logError(this.LOG_WARN,  flat(arguments)) } // LOG_DEBUG
	N.info  = function () { this.logError(this.LOG_INFO,  flat(arguments)) }
	N.warn  = function () { this.logError(this.LOG_WARN,  flat(arguments)) }
	N.error = function () { this.logError(this.LOG_ERR,   flat(arguments)) }
	self.log = function () { N.log.apply(N, arguments) }
	
	N.resultNames = {}
	var names = ["OK", "ERROR", "DONE", "AGAIN"]
	for (var i = 0; i < names.length; i++)
		N.resultNames[N[names[i]]] = names[i]
})();


// basic library loading
;(function(){

var JSLIB = environment.JSLIB, lib = self.lib = JSLIB ? String(JSLIB).split(':') : []
lib.unshift(__FILE__.replace(/\/[^\/]+$/, ''))
lib.required = {}
self.require = function (fname)
{
	// log('require ' + fname)
	if (lib.required[fname])
		return lib.required[fname]
	
	if (fname[0] === '/')
	{
		load(lib.required[fname] = fname)
		return fname
	}
	else
	{
		for (var i = 0; i < lib.length; i++)
		{
			var path = lib[i] + '/' + fname
			if (new File(path).exists)
			{
				load(lib.required[fname] = path)
				// log(fname + ' is loaded by path ' + path)
				return path
			}
		}
		throw 'Can`t find "' + fname + '" in [' + lib.join(', ') + ']'
	}
}

})();

require('proto.js')

// log('Nginx.js loaded')


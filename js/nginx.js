
// Nginx wrapers
;(function(){
	if (!self.Nginx)
		throw "global.Nginx is undefined"
	
	var Me = self.Nginx
	function flat (args) { return Array.prototype.slice.apply(args).join(", ") }
	Me.log   = function () { this.logError(this.LOG_WARN,  flat(arguments)) } // LOG_DEBUG
	Me.info  = function () { this.logError(this.LOG_INFO,  flat(arguments)) }
	Me.warn  = function () { this.logError(this.LOG_WARN,  flat(arguments)) }
	Me.error = function () { this.logError(this.LOG_ERR,   flat(arguments)) }
	self.log = function () { Me.log.apply(Me, arguments) }
	
	Me.resultNames = {}
	var names = ["OK", "ERROR", "DONE", "AGAIN"]
	for (var i = 0; i < names.length; i++)
		Me.resultNames[Me[names[i]]] = names[i]
})();


;(function(){
	if (!self.Nginx.Request)
		throw "global.Nginx.Request is undefined"
	
	var Me = self.Nginx.Request
	var slice = Array.prototype.slice
	Me.prototype.puts = function (str) { this.print(str + "\n") }
})();




// basic library loading
;(function(){

var JSLIB = environment.JSLIB, lib = self.lib = JSLIB ? String(JSLIB).split(":") : []
lib.unshift(__FILE__.replace(/\/[^\/]+$/, ""))
lib.required = {}
self.require = function (fname)
{
	// log("require " + fname)
	if (lib.required[fname])
		return lib.required[fname]
	
	if (fname[0] === "/")
	{
		load(lib.required[fname] = fname)
		return fname
	}
	else
	{
		for (var i = 0; i < lib.length; i++)
		{
			var path = lib[i] + "/" + fname
			try
			{
				load(path)
				return lib.required[fname] = path
			}
			catch (ex)
			{
				if (ex.message.indexOf("can't open") == -1)
					throw ex
			}
		}
		throw "Can`t find '" + fname + "' in [" + lib.join(", ") + "]"
	}
}

})();

require("proto.js")

// log("Nginx.js loaded")


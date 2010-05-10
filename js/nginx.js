;(function(){

self.ANSIColors =
{
	CLEAR:   '\x1B[0m',
	BRIGHT:  '\x1B[1m',
	FAINT:   '\x1B[2m',
	BLACK:   '\x1B[30m',
	RED:     '\x1B[31m',
	GREEN:   '\x1B[32m',
	YELLOW:  '\x1B[33m',
	BLUE:    '\x1B[34m',
	MAGENTA: '\x1B[35m',
	CYAN:    '\x1B[36m',
	WHITE:   '\x1B[37m',
	
	wrap: function (str, color)
	{
		return this[color] + str + this.CLEAR
	}
}

})();

// Nginx wrapers
;(function(){
	if (!self.Nginx)
		throw "global.Nginx is undefined"
	
	var Me = self.Nginx
	function flat (args) { return Array.prototype.slice.apply(args).join(", ") }
	Me.log   = function () { this.logError(this.LOG_WARN, ANSIColors.wrap(flat(arguments), 'GREEN')) } // LOG_DEBUG
	Me.info  = function () { this.logError(this.LOG_INFO, ANSIColors.wrap(flat(arguments), 'CLEAR')) }
	Me.warn  = function () { this.logError(this.LOG_WARN, ANSIColors.wrap(flat(arguments), 'CLEAR')) }
	Me.error = function () { this.logError(this.LOG_ERR,  ANSIColors.wrap(flat(arguments), 'CLEAR')) }
	self.log = function () { Me.log.apply(Me, arguments) }
	
	Me.resultNames = {}
	var names = ["OK", "ERROR", "DONE", "AGAIN"]
	for (var i = 0; i < names.length; i++)
		Me.resultNames[Me[names[i]]] = names[i]
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
				if (String(ex).indexOf("can't open") == -1)
					throw ex
			}
		}
		throw "Can`t find '" + fname + "' in [" + lib.join(", ") + "]"
	}
}

})();

require("prototype.js")
require("timers.js")

;(function(){
	if (!self.Nginx.Request)
		throw "global.Nginx.Request is undefined"
	
	var Me = self.Nginx.Request, proto = Me.prototype
	proto.puts = function (str) { this.print(str + "\n") }
	
	Me.mixIn(Timers)
})();


// log("Nginx.js loaded")


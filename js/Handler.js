require("cascade.js")
require("test.js")
require("test-tool.js")
require("tests.js")

var NginxTests = {}

;(function(){

var memory = []

var myName = 'Handler', Me =
{
	run: function (r)
	{
		var m = /([^\/]+)$/.exec(r.uri)
		
		if (m)
		{
			var method = m[1].replace(/-(\w)/, function (m) { return m[1].toUpperCase() })
			
			if (NginxTests[method])
				return NginxTests[method](r)
			else
				r.sendString('no such method')
		}
		else
			r.sendString('error in uri')
		
		return Nginx.OK
	}
}

Me.className = myName
self[myName] = Me

})();

require("tests/cascade-tests.js")
require("tests/timer-parallel.js")
require("tests/timer-order.js")
require("tests/timer-cascade.js")
require("tests/subrequest-quick.js")

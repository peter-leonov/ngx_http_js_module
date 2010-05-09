require('cascade.js')
require('test.js')
require('test-tool.js')
require('tests.js')
require('url-encode.js')
require('regexp-escape.js')

Test.Inspector.prototype.sep = ' '
Test.Inspector.prototype.indc = ''

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
			var method = m[1].replace(/-(\w)/g, function (m) { return m[1].toUpperCase() })
			
			if (NginxTests[method])
				return NginxTests[method](r)
			else
				r.sendString('no such method')
		}
		else
			r.sendString('error in uri')
		
		return Nginx.OK
	},
	
	variable: function (r)
	{
		return 123
	}
}

Me.className = myName
self[myName] = Me

})();

require('tests/nginx-object.js')
require('tests/request-object.js')
require('tests/request-body.js')
require('tests/request-body-file.js')
require('tests/file.js')
require('tests/process-cycle.js')
require('tests/request-cookies.js')
require('tests/request-headers-in.js')
require('tests/request-headers-in-set-get.js')
require('tests/variable-set.js')
// require('tests/cascade-tests.js')
// require('tests/timer-parallel.js')
// require('tests/timer-order.js')
// require('tests/timer-cascade.js')
// require('tests/subrequest-quick.js')

;(function(){

NginxTests.nginxObject = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	
	var args = UrlEncode.parse(r.args)
	
	Tests.test('tests for Nginx object', function (t)
	{
		t.type(Nginx, 'object', 'Nginx object')
		t.type(Nginx.logError, 'function', 'Nginx.logError')
		t.eq(Nginx.prefix, args.prefix, 'compare Nginx.prefix and args.prefix')
		t.eq(Nginx.pid, +args.pid, 'Nginx.pid')
		
		t.test('Nginx.time', function (t)
		{
			var timerPrecision = 250
			
			var jmsec = +new Date(),
				nmsec = Nginx.time
			
			t.peq(jmsec, nmsec, timerPrecision, 'new Date() == Nginx.time')
			
			function later (e)
			{
				var ljmsec = +new Date(),
					lnmsec = Nginx.time
				
				t.peq(ljmsec, lnmsec, timerPrecision, 'new Date() == Nginx.time')
				t.peq(ljmsec - jmsec, timerPrecision * 4, timerPrecision, 'diff within one second')
			}
			t.async(later, timerPrecision * 4)
		})
		
		t.test('Nginx.md5', function (t)
		{
			t.ok(Nginx.md5, 'Nginx.md5')
			t.eq(Nginx.md5(''), 'd41d8cd98f00b204e9800998ecf8427e', 'empty string')
			t.eq(Nginx.md5('abc'), '900150983cd24fb0d6963f7d28e17f72', 'abc')
			t.eq(Nginx.md5('абв'), '9817de3f4bd1d1d149fc366d46e5e134', '"абв" converted to UTF-8')
			var escaped = escape('абв')
			t.eq(escaped, '%D0%B0%D0%B1%D0%B2', 'escape("абв")')
			t.eq(Nginx.md5(escaped), 'c23766e4cb326ad8e9df8c0700ec28db', 'escaped "абв"')
		})
		
	})
	Tests.oncomplete = function ()
	{
		r.sendSpecial(Nginx.HTTP_LAST)
	}
	Tests.run(r)
	
	return Nginx.OK
}

})();
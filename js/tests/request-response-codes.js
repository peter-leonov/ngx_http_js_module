;(function(){

NginxTests.requestResponseCodes = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	
	Tests.test('tests for body callback result codes', function (t)
	{
		function test (name, res)
		{
			var code = Nginx[name]
			
			if (res === undefined)
				res = code
			
			t.test(name + ' (' + code + ')', function (t)
			{
				function callback (sr, body, rc)
				{
					if (rc != Nginx.OK)
						return
					
					t.eq(sr.headersOut.status, res, 'response status')
					
					t.done()
				}
				
				r.subrequest('/loopback/run/request-response-codes-handler?' + code, callback)
				
				t.wait(3000)
			})
		}
		
		test('HTTP_NO_CONTENT')
		test('HTTP_MOVED_PERMANENTLY')
		test('HTTP_MOVED_TEMPORARILY')
		test('HTTP_NOT_MODIFIED')
		
		test('HTTP_BAD_REQUEST')
		test('HTTP_UNAUTHORIZED')
		test('HTTP_FORBIDDEN')
		test('HTTP_NOT_FOUND')
		test('HTTP_NOT_ALLOWED')
		test('HTTP_CLOSE', 502)
		
		test('HTTP_INTERNAL_SERVER_ERROR')
		test('HTTP_NOT_IMPLEMENTED')
		test('HTTP_BAD_GATEWAY')
		test('HTTP_SERVICE_UNAVAILABLE')
		test('HTTP_GATEWAY_TIME_OUT')
		test('HTTP_INSUFFICIENT_STORAGE')
	})
	Tests.oncomplete = function ()
	{
		r.puts('all done')
		r.flush()
		r.sendSpecial(Nginx.HTTP_LAST)
	}
	Tests.run(r)
	
	return Nginx.OK
}

NginxTests.requestResponseCodesHandler = function (r)
{
	return +r.args
}

})();
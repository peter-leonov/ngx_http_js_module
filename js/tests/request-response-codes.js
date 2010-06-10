;(function(){

NginxTests.requestResponseCodes = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	
	Tests.test('tests for body callback result codes', function (t)
	{
		function test (code)
		{
			t.test('rc = ' + code, function (t)
			{
				function callback (sr, body, rc)
				{
					if (rc != Nginx.OK)
						return
					
					t.eq(sr.headersOut.status, code, 'response status')
					
					t.done()
				}
				
				r.subrequest('/loopback/run/request-response-codes-handler?' + code, callback)
				
				t.wait(3000)
			})
		}
		
		test(403)
		test(404)
		
		
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
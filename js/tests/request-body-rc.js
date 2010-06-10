;(function(){

NginxTests.requestBodyRC = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	
	Tests.test('tests for body callback result codes', function (t)
	{
		t.test('rc = 200', function (t)
		{
			function callback (sr, body, rc)
			{
				if (rc != Nginx.OK)
					return
				
				t.eq(sr.headersOut.status, 200, 'response status')
				
				t.done()
			}
			
			r.subrequest('/loopback/run/request-body-r-c-handler?200', callback)
			
			t.wait(3000)
		})
		
		
		
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

NginxTests.requestBodyRCHandler = function (r)
{
	function body ()
	{
		r.sendString(r.body)
		
		return Nginx.DONE
	}
	
	r.getBody(body)
}

})();
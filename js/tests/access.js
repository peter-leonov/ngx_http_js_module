;(function(){

NginxTests.access = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	
	Tests.test('tests for body callback result codes', function (t)
	{
		t.test('granted', function (t)
		{
			function callback (sr, body, rc)
			{
				if (rc != Nginx.OK)
					return
				
				t.eq(sr.headersOut.status, 404, 'response status')
				
				t.done()
			}
			
			r.subrequest('/loopback/run/access-path', callback)
			
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

NginxTests.accessCheck = function (r)
{
	return Nginx.DECLINED
}

})();
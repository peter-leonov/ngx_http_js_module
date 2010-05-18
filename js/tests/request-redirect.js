;(function(){

NginxTests.requestRedirect = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	
	Tests.test('redirect', function (t)
	{
		t.test('redirect to JS handler', function (t)
		{
			function callback (sr, body, rc)
			{
				t.eq(body, 'the body', 'response body')
				
				t.done()
			}
			
			r.subrequest('/loopback/run/request-redirect-handler', callback)
			
			t.wait(3000)
		})
		
		t.test('redirect to non existent uri', function (t)
		{
			function callback (sr, body, rc)
			{
				t.eq(body, '', 'response body')
				t.eq(sr.headersOut.status, 404, 'response status')
				t.eq(rc, Nginx.OK, 'response code')
				
				t.done()
			}
			
			r.subrequest('/loopback/run/request-redirect-handler-404', callback)
			
			t.wait(3000)
		})
	})
	Tests.oncomplete = function ()
	{
		r.puts('all done')
		r.sendSpecial(Nginx.HTTP_LAST)
	}
	Tests.run(r)
	
	return Nginx.OK
}


NginxTests.requestRedirectHandler = function (r)
{
	r.redirect('/run/request-redirect-target')
	return Nginx.DONE
}

NginxTests.requestRedirectTarget = function (r)
{
	r.sendHttpHeader('application/json')
	r.print('the body')
	r.sendSpecial(Nginx.HTTP_LAST)
	
	return Nginx.OK
}


NginxTests.requestRedirectHandler404 = function (r)
{
	r.redirect('/does-not-exist')
	return Nginx.DONE
}


})();
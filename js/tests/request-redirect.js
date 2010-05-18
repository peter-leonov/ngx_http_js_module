;(function(){

NginxTests.requestRedirect = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	
	Tests.test('sendfile', function (t)
	{
		function callback (sr, body, rc)
		{
			t.eq(body, 'the body', 'response body')
			
			t.done()
		}
		
		r.subrequest('/loopback/run/request-redirect-handler', callback)
		
		t.wait(3000)
	})
	Tests.oncomplete = function ()
	{
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

})();
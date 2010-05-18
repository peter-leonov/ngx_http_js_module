;(function(){

NginxTests.sendfile = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	
	Tests.test('sendfile', function (t)
	{
		function callback (sr, body, rc)
		{
			t.ok(body, 'response body')
			
			t.done()
		}
		
		r.subrequest('/loopback/run/sendfile-handler', callback)
		
		t.wait(3000)
	})
	Tests.oncomplete = function ()
	{
		r.sendSpecial(Nginx.HTTP_LAST)
	}
	Tests.run(r)
	
	return Nginx.OK
}

NginxTests.sendfileHandler = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	r.flush()
	
	// all these four commants add just a specific buffer to the output chain
	// r.sendfile('/etc/passwd')
	r.print('before:')
	r.flush()
	
	r.sendfile(Nginx.prefix + 'sendfile-1.txt')
	
	r.print(':after')
	r.flush()
	
	r.sendSpecial(Nginx.HTTP_LAST)
	
	return Nginx.OK
}

})();
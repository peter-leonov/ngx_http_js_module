;(function(){

NginxTests.sendfile = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	
	Tests.test('sendfile', function (t)
	{
		function callback (sr, body, rc)
		{
			t.eq(body, 'before:ha-ha, sendme!:abbbc:the rest:after', 'response body')
			
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
	
	// all these commants just add a specific buffer to the output chain
	r.print('before:')
	r.sendfile(Nginx.prefix + 'sendfile-1.txt')
	r.print(':')
	r.sendfile(Nginx.prefix + 'sendfile-2.txt', 2, 5)
	r.print(':')
	r.sendfile(Nginx.prefix + 'sendfile-3.txt', 3)
	r.print(':after')
	
	r.flush()
	r.sendSpecial(Nginx.HTTP_LAST)
	
	return Nginx.OK
}

})();
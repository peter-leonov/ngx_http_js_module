;(function(){

NginxTests.requestSendString = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	
	Tests.test('test sendString()', function (t)
	{
		function callback (sr, body, rc)
		{
			t.eq(sr.headersOut['Content-Type'], 'text/html; charset=utf-8', 'description')
			t.eq(body, 'o-la-la-la', 'response body')
			
			t.done()
		}
		
		r.subrequest('/loopback/run/request-send-string-handler', callback)
		
		t.wait(3000)
	})
	Tests.oncomplete = function ()
	{
		r.sendSpecial(Nginx.HTTP_LAST)
	}
	Tests.run(r)
	
	return Nginx.OK
}

NginxTests.requestSendStringHandler = function (r)
{
	r.sendString('o-la-la-la', 'text/html; charset=utf-8')
	return Nginx.OK
}

})();
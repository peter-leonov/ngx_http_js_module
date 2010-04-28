;(function(){

NginxTests.requestBodyFile = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	
	Tests.test('tests for body receiving', function (t)
	{
		t.expect(6)
		
		var args = UrlEncode.parse(r.args)
		
		t.eq(r.method, 'POST', 'hasBody')
		t.eq(r.hasBody, true, 'hasBody')
		t.eq(r.body, undefined, 'body')
		
		if (!r.hasBody)
			return
		
		function body ()
		{
			t.eq(r.body, undefined, 'body')
			t.ok(r.bodyFilename, 'bodyFilename')
			// getBody() may run callback synchronously
			// if all the body has been got in the first packet
			// with headers
			t.async(function (t) { t.done() }, 10)
		}
		
		var rc = r.getBody(body)
		t.eq(rc, Nginx.OK, 'getBody()')
		
		t.wait(10000)
	})
	Tests.oncomplete = function ()
	{
		r.sendSpecial(Nginx.HTTP_LAST)
	}
	Tests.run(r)
	
	return Nginx.OK
}

})();
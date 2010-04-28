;(function(){

NginxTests.requestBodyFile = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	
	Tests.test('tests for body receiving', function (t)
	{
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
			t.done()
		}
		
		var rc = r.getBody(body)
		t.ok(rc, 'getBody()')
		
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
;(function(){

NginxTests.requestBodyFile = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	
	Tests.test('tests for body in file receiving', function (t)
	{
		t.expect(8)
		
		var args = UrlEncode.parse(r.args),
			received = false
		
		t.eq(r.method, 'POST', 'hasBody')
		t.eq(r.hasBody, true, 'hasBody')
		t.eq(r.body, undefined, 'body')
		t.eq(r.bodyFilename, undefined, 'bodyFilename')
		t.gt(r.headersIn['Content-Length'], 100000, 'Content-Length')
		
		if (!r.hasBody)
			return
		
		function body ()
		{
			t.eq(r.body, undefined, 'body')
			t.ok(r.bodyFilename, 'bodyFilename')
			t.done()
		}
		
		var rc = r.getBody(body)
		
		if (t.finished)
		{
			t.warn('strangely fast body receiving')
		}
		else
		{
			t.eq(rc, Nginx.AGAIN, 'Nginx.AGAIN')
			t.wait(5000)
		}
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

})();
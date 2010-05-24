;(function(){

NginxTests.requestBodyDiscard = function (r)
{
	var discardBody = r.discardBody()
	
	r.sendHttpHeader('text/plain; charset=utf-8')
	
	Tests.test('tests for body discarding', function (t)
	{
		t.expect(8)
		
		t.eq(discardBody, Nginx.OK, 'first discardBody()')
		t.eq(r.method, 'POST', 'hasBody')
		t.eq(r.hasBody, true, 'hasBody')
		t.eq(r.body, undefined, 'body')
		
		function body ()
		{
			t.eq(r.hasBody, true, 'hasBody')
			t.no(r.bodyFilename, 'bodyFilename')
			t.async(function (t) { t.done() }, 10)
		}
		
		t.eq(r.discardBody(), Nginx.OK, 'second discardBody()')
		
		var rc = r.getBody(body)
		t.eq(rc, Nginx.OK, 'getBody()')
		
		t.wait(5000)
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
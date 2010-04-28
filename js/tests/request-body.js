;(function(){

NginxTests.requestBody = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	
	Tests.test('tests for body receiving', function (t)
	{
		t.expect(5)
		var args = UrlEncode.parse(r.args)
		
		t.eq(r.method, 'POST', 'hasBody')
		t.eq(r.hasBody, true, 'hasBody')
		t.eq(r.body, undefined, 'body')
		
		function body ()
		{
			t.match(r.body, new RegExp(RegExp.escape(args.body)), 'body')
			// getBody() may run callback synchronously
			// if all the body has been got in the first packet
			// with headers
			t.async(function (t) { t.done() }, 10)
		}
		
		var rc = r.getBody(body)
		t.eq(rc, Nginx.OK, 'rc')
		
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
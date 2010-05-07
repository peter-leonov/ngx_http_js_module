;(function(){

NginxTests.requestHeadersIn = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	
	var args = JSON.parse(r.args.replace(/\+/g, ' '))
	
	Tests.test('tests for request headers processing', function (t)
	{
		var h = r.headersIn
		
		t.ok(args, 'args')
		t.ok(h, 'headersIn')
		
		for (var k in args)
		{
			t.eq(typeof args[k] == 'number' ? +h[k] : h[k], args[k], k)
			t.eq(typeof args[k] == 'number' ? +h[k] : h[k.toUpperCase()], args[k], k.toUpperCase())
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
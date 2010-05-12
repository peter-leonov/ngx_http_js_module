;(function(){

NginxTests.requestHeadersOutSetGet = function (r)
{
	// r.headersOut['Server'] = 'trololo'
	// r.headersOut['Server'] = 'trulala'
	r.sendHttpHeader('text/plain; charset=utf-8')
	
	Tests.test('tests for r.headersOut setting and getting', function (t)
	{
		var h = r.headersOut
		
		t.ok(h, 'headersOut')
		
		function testHeader (t, h, name)
		{
			t.eq(h[name], undefined, 'initial')
			h[name] = 'abc'
			t.eq(h[name], 'abc', 'first set')
			h[name] = undefined
			t.eq(h[name], undefined, 'first delete')
			h[name] = 'def'
			t.eq(h[name], 'def', 'second set')
			
		}
		
		var headers = ['Server']
		
		for (var i = 0; i < headers.length; i++)
		{
			var header = headers[i]
			t.test(header, function (t)
			{
				testHeader(t, h, header)
			})
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
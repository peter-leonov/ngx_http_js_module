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
		
		function testHeader (t, h, header)
		{
			var name = header.name,
				value = header.value
			
			t.eq(h[name], undefined, 'initial')
			
			h[name] = value
			t.eq(h[name], value, 'first set')
			
			var nameN = header.nameN
			if (nameN)
				t.eq(h[nameN], header.valueN, 'number value (' + nameN + ')')
			
			h[name] = undefined
			t.eq(h[name], undefined, 'first delete')
			
			h[name] = value
			t.eq(h[name], value, 'second set')
			
			h[name] = ''
			t.eq(h[name], undefined, 'disabled')
			
			h[name] = value
			t.eq(h[name], value, 'third set')
		}
		
		var headers =
		[
			{name: 'Server', value: 'nginxy'},
			{name: 'Date', value: 'Wed, 12 May 2010 19:15:52 GMT', nameN: '$dateTime', valueN: 1273691752}
		]
		
		for (var i = 0; i < headers.length; i++)
		{
			(function (header)
			{
				t.test(header.name, function (t)
				{
					testHeader(t, h, header)
				})
			})(headers[i])
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
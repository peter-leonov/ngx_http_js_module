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
				value = header.value,
				nameN = header.nameN
			
			t.eq(h[name], undefined, 'initial')
			
			h[name] = value
			t.eq(h[name], value, 'first set')
			
			if (nameN)
				t.eq(h[nameN], header.valueN, 'number value (' + nameN + ')')
			
			h[name] = undefined
			t.eq(h[name], undefined, 'first delete')
			
			if (nameN)
				t.eq(h[nameN], header.deletedN, 'deleted number value (' + nameN + ')')
			
			h[name] = value
			t.eq(h[name], value, 'second set')
			
			if (nameN)
				t.eq(h[nameN], header.valueN, 'number value (' + nameN + ')')
			
			h[name] = ''
			t.eq(h[name], undefined, 'disabled')
			
			if (nameN)
				t.eq(h[nameN], header.deletedN, 'disabled number value (' + nameN + ')')
			
			h[name] = value
			t.eq(h[name], value, 'third set')
			
			if (nameN)
				t.eq(h[nameN], header.valueN, 'number value (' + nameN + ')')
			
			h[name] = undefined
			t.eq(h[name], undefined, 'second delete')
		}
		
		var headers =
		[
			{name: 'Server', value: 'nginxy'},
			{name: 'Date', value: 'Wed, 12 May 2010 19:15:52 GMT', nameN: '$dateTime', valueN: 1273691752, deletedN: -1},
			{name: 'Content-Length', value: '123456', nameN: '$contentLength', valueN: 123456, deletedN: 0},
			{name: 'Content-Length', value: '1099511627776', nameN: '$contentLength', valueN: 1099511627776, deletedN: 0},
			{name: 'Content-Encoding', value: 'gzip'},
			{name: 'Location', value: 'http://nginx.org/'},
			{name: 'Location', value: '/local-redirect'},
			{name: 'Refresh', value: '0'},
			{name: 'Refresh', value: '0; url=http://nginx.org/'},
			{name: 'Last-Modified', value: 'Wed, 12 May 2010 19:15:52 GMT', nameN: '$lastModified', valueN: 1273691752, deletedN: -1},
			{name: 'Content-Range', value: 'bytes 500-1233/1234'},
			{name: 'Accept-Ranges', value: 'bytes'},
			{name: 'WWW-Authenticate', value: 'Basic realm="Nginx Area"'},
			{name: 'Expires', value: 'Wed, 12 May 2010 19:15:52 GMT'},
			{name: 'ETag', value: '8f1b0fb0a9f67ffaa43a83cad28435ca'}
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
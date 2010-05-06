;(function(){

NginxTests.requestCookies = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	
	var args = UrlEncode.parse(r.args)
	
	var Cookies = Nginx.Cookies
	
	Tests.test('tests for cookies processing', function (t)
	{
		t.test('Cookies class', function (t)
		{
			t.exception(function (t)
			{
				new Cookies()
			}, 'new Cookies()')
			
			t.exception(function (t)
			{
				Cookies.prototype.empty.call({})
			}, 'call({})')
			
			t.exception(function (t)
			{
				Cookies.prototype.empty.call(r)
			}, 'call(r)')
		})
		
		t.test('headersIn.cookies', function (t)
		{
			t.ok(r.headersIn['Cookie'], 'Cookie header')
			
			var cookies = r.headersIn.cookies
			t.ok(cookies, 'headersIn.cookies')
			
			var count = +args.count
			
			t.eq(cookies.length, count, 'length')
			
			for (var i = 1; i <= count; i++)
				t.eq(cookies[args['name' + i]], args['value' + i], 'name' + i)
			
			t.eq(cookies['lalala'], undefined, 'lalala')
		})
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
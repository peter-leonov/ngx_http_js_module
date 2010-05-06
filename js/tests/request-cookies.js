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
			
			t.eq(cookies.length, 2, 'length')
			
			t.eq(cookies[args.name1], args.value1, 'name1')
			t.eq(cookies[args.name2], args.value2, 'name2')
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
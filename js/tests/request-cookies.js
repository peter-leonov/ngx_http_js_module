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

		})
		
		t.test('headersIn.cookies', function (t)
		{
			t.ok(r.headersIn['Cookie'], 'Cookie header')
			
			var cookies = r.headersIn.cookies
			t.ok(cookies, 'headersIn.cookies')
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
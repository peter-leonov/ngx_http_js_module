;(function(){

NginxTests.subrequests = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	r.flush()
	
	Tests.test('tests for subrequests', function (t)
	{
		t.test('quick', function (t)
		{
			t.mayFail()
			
			function callback (sr, body, rc)
			{
				t.match(String(body), /403 Forbidden/i, 'body')
				t.eq(rc, 403, 'rc')
				
				t.done()
			}
			
			r.subrequest('/quick', callback)
			t.wait(3000)
		})
		
		
		t.test('quick with NGX_DONE', function (t)
		{
			function callback (sr, body, rc)
			{
				t.eq(body, undefined, 'body')
				t.eq(rc, 403, 'rc')
				
				t.done()
				return Nginx.DONE
			}
			
			r.subrequest('/quick', callback)
			t.wait(3000)
		})
		
		
		t.test('slow', function (t)
		{
			function callback (sr, body, rc)
			{
				t.match(String(body), /welcome to nginx/i, 'body')
				
				t.done()
			}
			
			r.subrequest('/loopback/', callback)
			t.wait(3000)
		})
		
		
		t.test('JS handler', function (t)
		{
			function callback (sr, body, rc)
			{
				t.eq(sr.headersOut['X-Lalala'], 'lololo', 'headersOut["X-Lalala"]')
				
				t.done()
			}
			
			// r.setTimeout(function () { t.done() }, 500)
			// t.wait(3000)
			
			r.subrequest('/run/subrequests-headers', callback)
			t.wait(3000)
		})
		
		
		t.test('JS handler via proxy pass', function (t)
		{
			function callback (sr, body, rc)
			{
				t.eq(sr.headersOut['X-Lalala'], 'lololo', 'headersOut["X-Lalala"]')
				
				t.done()
			}
			
			// r.setTimeout(function () { t.done() }, 500)
			// t.wait(3000)
			
			r.subrequest('/loopback/run/subrequests-headers', callback)
			t.wait(3000)
		})
	})
	Tests.oncomplete = function ()
	{
		r.puts('all done')
		r.flush()
	}
	Tests.run(r)
	
	r.oncleanup = function ()
	{
		this.puts('cleanup')
		this.flush()
		this.sendSpecial(Nginx.HTTP_LAST)
	}
	
	return Nginx.OK
}

NginxTests.subrequestsHeaders = function (r)
{
	r.headersOut['X-Lalala'] = 'lololo'
	r.sendHttpHeader('text/plain; charset=utf-8')
	r.print('the body')
	r.flush()
	r.print('the body')
	r.flush()
	r.print('the body')
	r.flush()
	r.sendSpecial(Nginx.HTTP_LAST)
	
	// return Nginx.DONE to avoid subrequest handler
	return Nginx.OK
}

})();
;(function(){

NginxTests.subrequests = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	r.flush()
	
	Tests.test('tests for subrequests', function (t)
	{
		t.test('quick', function (t)
		{
			function callback (sr, body, rc)
			{
				var body = String(body)
				r.puts('callback with rc=' + rc + ', body=' + JSON.stringify(body) + ', header=' + sr.headersOut['X-Lalala'])
				r.flush()
				t.done()
				
				return Nginx.DONE
			}
			
			// r.setTimeout(function () { t.done() }, 500)
			// t.wait(3000)
			
			r.subrequest('/quick', callback)
			t.wait(3000)
		})
		
		t.test('slow', function (t)
		{
			function callback (sr, body, rc)
			{
				var body = String(body)
				r.puts('callback with rc=' + rc + ', body=' + JSON.stringify(body) + ', header=' + sr.headersOut['X-Lalala'])
				r.flush()
				t.done()
				
				return Nginx.DONE
			}
			
			// r.setTimeout(function () { t.done() }, 500)
			// t.wait(3000)
			
			r.subrequest('/slow', callback)
			t.wait(3000)
		})
		
		t.test('/run/subrequests-headers', function (t)
		{
			function callback (sr, body, rc)
			{
				var body = String(body)
				r.puts('callback with rc=' + rc + ', body=' + JSON.stringify(body) + ', header=' + sr.headersOut['X-Lalala'])
				r.flush()
				t.done()
				
				return Nginx.DONE
			}
			
			// r.setTimeout(function () { t.done() }, 500)
			// t.wait(3000)
			
			r.subrequest('/slow', callback)
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
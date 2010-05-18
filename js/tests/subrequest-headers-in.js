;(function(){

NginxTests.subrequestHeadersIn = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	r.flush()
	
	Tests.test('tests for headersIn in subrequest', function (t)
	{
		function callback (sr, body, rc)
		{
			t.ok(body, 'response body')
			
			r.print(body)
			
			t.done()
		}
		
		r.headersIn['X-La-la-la'] = 'lololo'
		
		var sr = r.subrequest('/loopback/run/subrequest-headers-in-handler', callback)
		
		h = sr.headersIn
		t.ok(h, 'sr.headersIn')
		
		// h['X-La-la-la'] = 'lololo'
		
		t.wait(3000)
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

NginxTests.subrequestHeadersInHandler = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	r.flush()
	
	Tests.test('tests for headersIn via subrequest', function (t)
	{
		var h = r.headersIn
		t.eq(h['X-La-la-la'], 'lololo', 'X-La-la-la')
	})
	// Tests.oncomplete = function ()
	// {
	// 	r.puts('all done')
	// 	r.flush()
	// }
	Tests.run(r)
	
	r.oncleanup = function ()
	{
		this.puts('cleanup')
		this.flush()
		this.sendSpecial(Nginx.HTTP_LAST)
	}
	
	return Nginx.OK
}

})();
;(function(){

NginxTests.subrequestHeadersOut = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	r.flush()
	
	Tests.test('tests for subrequests', function (t)
	{
		t.test('JS handler via proxy pass', function (t)
		{
			function callback (sr, body, rc)
			{
				t.eq(rc, Nginx.OK, 'rc == NGX_OK')
				
				t.eq(sr.headersOut.status, 206, 'headersOut.status')
				t.eq(sr.headersOut.statusLine, '206 Dont worry be happy', 'headersOut.statusLine')
				t.eq(sr.headersOut['X-Lalala'], 'lololo', 'headersOut["X-Lalala"]')
				t.eq(sr.headersOut['Content-Type'], 'application/json; charset=utf-8', 'headersOut["Content-Type"]')
				t.eq(sr.headersOut['Expires'], 'today', 'headersOut["Expires"]')
				
				t.eq(body, '12345', 'body')
				
				t.done()
			}
			
			r.subrequest('/loopback/run/subrequest-headers-out-handler', callback)
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

NginxTests.subrequestHeadersOutHandler = function (r)
{
	r.headersOut.status = 206
	r.headersOut.statusLine = '206 Dont worry be happy'
	r.headersOut['Range'] = 'bytes; 5-25'
	r.headersOut['X-Lalala'] = 'lololo'
	r.headersOut['Content-Type'] = 'application/json; charset=utf-8'
	r.headersOut['Expires'] = 'today'
	
	r.sendHttpHeader()
	
	r.print('12345')
	r.sendSpecial(Nginx.HTTP_LAST)
	
	// return Nginx.DONE to avoid subrequest handler
	return Nginx.OK
}

})();
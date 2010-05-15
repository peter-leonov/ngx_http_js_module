;(function(){

NginxTests.subrequests = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	
	// Tests.test('tests for subrequests', function (t)
	// {
		function callback (sr, body, rc)
		{
			log('\x1B[31m!!!\x1B[0m ' + rc)
			var body = String(body)
			r.puts('callback with rc=' + rc + ', body=' + JSON.stringify(body) + ', header=' + sr.headersOut['X-Lalala'])
			r.flush()
			// t.done()
		}
		
		r.subrequest('/quick', callback)
		r.subrequest('/slow', callback)
		r.subrequest('/run/subrequests-headers', callback)
		
		// t.wait(3000)
	// })
	// Tests.oncomplete = function ()
	// {
	// 	r.puts('all done')
	// 	r.flush()
	// 	r.sendSpecial(Nginx.HTTP_LAST)
	// }
	// Tests.run(r)
	
	r.oncleanup = function ()
	{
		this.puts('all done')
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
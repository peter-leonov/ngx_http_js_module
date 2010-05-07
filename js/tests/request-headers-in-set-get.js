;(function(){

NginxTests.requestHeadersInSetGet = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	
	Tests.test('tests for request headers processing', function (t)
	{
		var h = r.headersIn
		
		t.ok(h, 'headersIn')
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
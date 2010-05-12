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
		
		t.eq(h.Server, 'nginx', 'headersOut.Server')
		
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
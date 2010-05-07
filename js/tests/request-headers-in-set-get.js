;(function(){

NginxTests.requestHeadersInSetGet = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	
	Tests.test('tests for r.headersIn setting and getting', function (t)
	{
		var h = r.headersIn
		
		t.ok(h, 'headersIn')
		
		t.eq(h['Content-Length'], undefined, 'Content-Length')
		t.eq(h.$contentLengthN, -1, '$contentLengthN')
		h['Content-Length'] = 5555555555555555
		t.eq(h['Content-Length'], '5555555555555555', 'Content-Length')
		t.eq(h.$contentLengthN, 5555555555555555, '$contentLengthN')
		
		t.eq(h['Range'], undefined, 'Range')
		h['Range'] = 'bytes=1-2'
		t.eq(h['Range'], 'bytes=1-2', 'Range')
		
		t.ok(h['Host'], 'Host')
		h['Host'] = 'nginx.org:80'
		t.eq(h['Host'], 'nginx.org:80', 'Host')
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
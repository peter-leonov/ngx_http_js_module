;(function(){

NginxTests.requestObject = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	
	Tests.test('tests for Nginx.Request object', function (t)
	{
		t.ok(r.args, 'arguments')
		
		var args = UrlEncode.parse(r.args)
		
		t.eq(r.uri, args.uri, 'uri')
		t.eq(r.method, args.method, 'method')
		t.eq(r.filename, Nginx.prefix + 'html/run/request-object', 'filename')
		t.eq(r.remoteAddr, '127.0.0.1', 'remoteAddr')
		t.eq(r.headerOnly, false, 'headerOnly')
		t.eq(r.requestBody, undefined, 'requestBody')
	})
	Tests.oncomplete = function ()
	{
		r.sendSpecial(Nginx.HTTP_LAST)
	}
	Tests.run(r)
	
	return Nginx.OK
}

})();
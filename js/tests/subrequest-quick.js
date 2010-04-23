;(function(){

NginxTests.subrequestQuick = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	r.flush()
	
	Tests.test('cascade tests', function (t)
	{
		function callback (body, rc)
		{
			log('\x1B[31m!!!\x1B[0m')
			body = String(body)
			// r.print('callback with body="' + body + '", length=' + body.length + '\n')
			// r.flush()
			
			t.done()
		}
		
		r.subrequest('/quick', callback)
		
		t.wait(3000)
	})
	Tests.oncomplete = function ()
	{
		r.sendSpecial(Nginx.HTTP_LAST)
	}
	Tests.run(r)
	
	return Nginx.OK
}

})();
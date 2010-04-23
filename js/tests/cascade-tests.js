;(function(){

NginxTests.cascadeTests = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	r.flush()
	
	Tests.test('empty test', function (t)
	{
		t.ok(Nginx, 'Nginx present')
		
		t.test('headers out', function (t)
		{
			t.wait(3000)
		})
		
	})
	Tests.oncomplete = function ()
	{
		r.sendSpecial(Nginx.HTTP_LAST)
	}
	Tests.run(r)
	
	return Nginx.OK
}

})();
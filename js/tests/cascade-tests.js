;(function(){

NginxTests.cascadeTests = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	r.flush()
	
	Tests.test('cascade tests', function (t)
	{
		t.ok(Nginx, 'Nginx present')
		
		t.test('async() and wait()', function (t)
		{
			t.async(function (t) { t.ok(true, 'async() works'); t.done() }, 10)
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
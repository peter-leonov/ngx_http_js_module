;(function(){

var Dir = Nginx.Dir,
	prefix = Nginx.prefix

NginxTests.dir = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	
	Tests.test('tests for Nginx.Dir class', function (t)
	{
		t.type(Dir, 'function', 'Nginx.Dir object')
		
		
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
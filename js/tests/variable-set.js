;(function(){

NginxTests.variableSet = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	
	var args = UrlEncode.parse(r.args)
	
	Tests.test('tests for js_set', function (t)
	{
		t.eq(args.foo1, 'bar1', 'args.foo1')
		t.eq(args.foo2, 'null', 'args.foo2')
		t.eq(args.foo3, '', 'args.foo3')
		t.eq(args.foo4, '123', 'args.foo4')
	})
	Tests.oncomplete = function ()
	{
		r.sendSpecial(Nginx.HTTP_LAST)
	}
	Tests.run(r)
	
	return Nginx.OK
}

})();
;(function(){

NginxTests.variableSet = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	
	var args = UrlEncode.parse(r.args)
	
	Tests.test('tests for js_set', function (t)
	{
		t.eq(args.string, 'bar1', 'args.string')
		t.eq(args['null'], 'null', 'args.null')
		t.eq(args['undefined'], '', 'args.undefined')
		t.eq(args.number, '123', 'args.number')
		t.eq(args.property, 'property', 'args.property')
		t.eq(r.property, 'property', 'r.property')
	})
	Tests.oncomplete = function ()
	{
		r.sendSpecial(Nginx.HTTP_LAST)
	}
	Tests.run(r)
	
	return Nginx.OK
}

})();
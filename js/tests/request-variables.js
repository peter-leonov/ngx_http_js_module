;(function(){

NginxTests.requestVariables = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	
	var args = UrlEncode.parse(r.args)
	
	Tests.test('tests for request.variables', function (t)
	{
		var vars = r.variables
		
		t.ok(vars, 'r.variables')
		
		t.eq(vars.lalala, undefined, 'vars.lalala')
		
		t.eq(vars.js_set_number, '123', 'vars.js_set_number')
		
		t.eq(vars.js_request_variables_a, 'abc', 'vars.js_request_variables_a')
		t.eq(vars.js_request_variables_b, vars.js_request_variables_a + '123', 'vars.js_request_variables_b')
		t.eq(vars.js_request_variables_j, 'xxbar1xx', 'vars.js_request_variables_j')
		
		vars[0] = '0'
		t.eq(vars[0], '0', 'vars[0]')
		
		vars.foo1 = 'bar1'
		t.eq(vars.foo1, 'bar1', 'vars.foo1')
	})
	Tests.oncomplete = function ()
	{
		r.sendSpecial(Nginx.HTTP_LAST)
	}
	Tests.run(r)
	
	return Nginx.OK
}

})();
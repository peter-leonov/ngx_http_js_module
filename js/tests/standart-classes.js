;(function(){

NginxTests.standartClasses = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	
	Tests.test('tests for Nginx object', function (t)
	{
		t.eq('123'.utf8length, 3, '"123".utf8length')
		t.eq('xxxccc'.utf8length, 6, '"xxxccc".utf8length')
		t.eq('ляляля'.utf8length, 12, '"ляляля".utf8length')
	})
	Tests.oncomplete = function ()
	{
		r.sendSpecial(Nginx.HTTP_LAST)
	}
	Tests.run(r)
	
	return Nginx.OK
}

})();
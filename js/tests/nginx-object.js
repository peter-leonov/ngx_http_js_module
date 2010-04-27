;(function(){

NginxTests.nginxObject = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	
	Tests.test('tests for Nginx object', function (t)
	{
		t.type(Nginx, 'object', 'Nginx present')
		
		t.test('Nginx.time', function (t)
		{
			var timerPrecision = 250
			
			var jmsec = +new Date(),
				nmsec = Nginx.time
			
			t.peq(jmsec, nmsec, timerPrecision, 'new Date() == Nginx.time')
			
			function later (e)
			{
				var ljmsec = +new Date(),
					lnmsec = Nginx.time
				
				t.peq(ljmsec, lnmsec, timerPrecision, 'new Date() == Nginx.time')
				t.peq(ljmsec - jmsec, timerPrecision * 4, timerPrecision, 'diff within one second')
			}
			t.async(later, timerPrecision * 4)
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
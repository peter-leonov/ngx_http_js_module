;(function(){

NginxDemos.delayedOutput2 = function (r)
{
	r.sendHttpHeader('text/html')
	
	var count = 10
	
	function sayHello ()
	{
		r.print('Hello # ' + count + '!\n')
		r.flush()
		
		if (--count > 0)
			r.setTimer(sayHello, 250)
		else
			r.sendSpecial(Nginx.HTTP_LAST)
	}
	
	sayHello()
	
	return Nginx.OK
}

})();
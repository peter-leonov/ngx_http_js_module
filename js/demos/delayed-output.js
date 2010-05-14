;(function(){

NginxDemos.delayedOutput = function (r)
{
	r.sendHttpHeader('text/html; charset=utf-8')
	r.flush()
	
	var counter = 0
	
	function sayHello (e)
	{
		r.puts('Hello (' + ++counter + ')!')
		r.flush()
	}
	
	function done (e)
	{
		r.puts('Done.')
		r.sendSpecial(Nginx.HTTP_LAST)
	}
	
	r.setTimeout(sayHello, 1000)
	r.setTimeout(sayHello, 3000)
	r.setTimeout(sayHello, 5000)
	r.setTimeout(done, 7000)
	
	return Nginx.OK
}

})();
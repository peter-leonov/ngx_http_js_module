;(function(){

NginxTests.timerParallel = function (r)
{
	function callback ()
	{
		r.puts('callback')
		r.flush()
	}
	
	r.sendHttpHeader('text/plain; charset=utf-8')
	
	r.setTimer(callback, 500)
	r.setTimer(callback, 500)
	r.setTimer(callback, 500)
	r.setTimer(callback, 500)
	r.setTimer(callback, 500)
	r.setTimer(callback, 500)
	
	return Nginx.DONE
}

})();
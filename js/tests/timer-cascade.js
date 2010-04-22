;(function(){

NginxTests.timerCascade = function (r)
{
	var count = r.args ? +r.args : 3, total = 0
	function walk ()
	{
		total++
		
		r.print(count + '\n')
		r.flush()
		
		if (count <= 0)
			throw 'count <= 0'
		else if (count == 1)
		{
			r.print('print called ' + total + ' times\n')
			r.sendSpecial(Nginx.HTTP_LAST)
		}
		else
			r.setTimer(walk, 500)
		
		count--
	}
	
	r.sendHttpHeader('text/plain; charset=utf-8')
	walk()
	
	return Nginx.DONE
}

})();
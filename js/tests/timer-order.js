;(function(){

NginxTests.timerOrder = function (r)
{
	var order = [], count = 9
	function handler (num)
	{
		order.push(num)
		r.print(num + " has expired\n")
		r.flush()
		if (--count == 0)
		{
			var orderStr = JSON.stringify(order),
				rightStr = JSON.stringify(right)
			r.print("all done: " + orderStr + "\n")
			r.print(orderStr == rightStr ? "order passed\n" : "order failed, the right is " + rightStr)
			r.sendSpecial(Nginx.HTTP_LAST)
		}
	}
	
	r.sendHttpHeader("text/plain")
	r.print("it has begun!\n")
	r.flush()
	
	r.setTimeout(function () { handler(1) }, 0)
	r.setTimeout(function () { handler(2) }, 0)
	r.setTimeout(function () { handler(3) }, 0)
	r.setTimeout(function () { handler(4) }, 10)
	r.setTimeout(function () { handler(5) }, 5)
	r.setTimeout(function () { handler(6) }, 2000)
	r.setTimeout(function () { handler(7) }, 1000)
	r.setTimeout(function () { handler(8) }, 0)
	r.setTimeout(function () { handler(9) }, 1001)
	
	var right = [1, 2, 3, 8, 5, 4, 7, 9, 6]
	
	return Nginx.DONE
}

})();
var Handler =
{
	processRequest: function (r)
	{
		// log("processRequest")
		// r.sendHttpHeader("text/html; charset=utf-8")
		
		// very quick way to send some fixed length response
		// printOnlyString() sends hrader for you
		r.printOnlyString("Привет, Девелопер!\nТы вызвал страницу " + r.uri + ", методом " + r.method + ", с IP " + r.remoteAddr + "\n")
		r.sendSpecial(Nginx.HTTP_FLUSH)
		
		// log(rc == Nginx.OK)
		
		// log('processRequest')
		
		// function callback (sr, body)
		// {
		// 	log(sr.uri + " is loaded: " + body.length + " bytes")
		// 	r.printString(body)
		// 	r.sendSpecial(Nginx.HTTP_LAST)
		// }
		// function callback (sr, body) {}
		// var res = r.request("/lib", callback)
		// log("r.request() = " + Nginx.resultNames[res])
		// return res
		
		// var res = r.request("/lib")
		// log("r.request() = " + Nginx.resultNames[res])
		// r.sendSpecial(Nginx.HTTP_LAST)
		
		
		// return Nginx.AGAIN
		return Nginx.OK
		// return Nginx.DONE
		// return Nginx.ERROR
	}
}

// Nginx.Request.prototype.cleanup = function () { log("cleanup(): " + this.uri) }
// Nginx.Request.prototype.cleanup = function () { log("cleanup") }
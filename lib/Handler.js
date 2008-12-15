;(function(){

var cache = ""

self.Handler =
{
	processRequest: function (r)
	{
		if (cache)
		{
			r.printOnlyString(cache, "text/plain")
			r.sendSpecial(Nginx.HTTP_FLUSH)
			return Nginx.OK
		}
		
		function callback (sr, body)
		{
			log(sr.uri + " is loaded: " + body.length + " bytes")
			cache = body
			r.printOnlyString(cache)
			r.sendSpecial(Nginx.HTTP_FLUSH)
		}
		return r.request("/lib", callback)
	}
	
	// processRequest: function (r)
	// {
	// 	// very quick way to send some fixed length response
	// 	// printOnlyString() sets the Content-length, sends hraders and flushes data for you
	// 	r.printOnlyString("Привет, Девелопер!")
	// 	// r.sendSpecial(Nginx.HTTP_FLUSH)
	// 	
	// 	return Nginx.OK
	// }
	
	// processRequest: function (r)
	// {
	// 	log("processRequest")
	// 	r.sendHttpHeader("text/html; charset=utf-8")
	// 	r.printString("Привет, Девелопер!\nТы вызвал страницу " + r.uri + ", методом " + r.method + ", с IP " + r.remoteAddr + "\n")
	// 	r.sendSpecial(Nginx.HTTP_FLUSH)
	// 	return Nginx.OK
	// }
	
	// processRequest: function (r)
	// {
	// 	log("processRequest")
	// 	r.sendHttpHeader("text/html; charset=utf-8")
	// 	r.printString("Привет, Девелопер!\nТы вызвал страницу " + r.uri + ", методом " + r.method + ", с IP " + r.remoteAddr + "\n")
	// 	r.sendSpecial(Nginx.HTTP_FLUSH)
	// 	
	// 	// log(rc == Nginx.OK)
	// 	
	// 	// log('processRequest')
	// 	
	// 	function callback (sr, body)
	// 	{
	// 		log(sr.uri + " is loaded: " + body.length + " bytes")
	// 		r.printOnlyString(body)
	// 		r.sendSpecial(Nginx.HTTP_FLUSH)
	// 	}
	// 	var res = r.request("/lib", callback)
	// 	log("r.request() = " + Nginx.resultNames[res])
	// 	return res
	// 	
	// 	// return Nginx.AGAIN
	// 	return Nginx.OK
	// 	// return Nginx.DONE
	// 	// return Nginx.ERROR
	// }
}

// Nginx.Request.prototype.cleanup = function () { log("cleanup(): " + this.uri) }
// Nginx.Request.prototype.cleanup = function () { log("cleanup") }

})();
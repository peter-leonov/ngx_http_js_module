;(function(){

var cache = ""

self.Handler =
{
	processRequest: function (r)
	{
		log("r.headersOut['Accept'] ==> ", r.headersOut['Accept'])
		log("r.headersOut['X-JS'] = '1.70' ==> ", r.headersOut['X-JS'] = '1.70')
		log("r.headersOut['X-JS'] ==> ", r.headersOut['X-JS'])
		
		r.sendHttpHeader("text/html; charset=utf-8")
		r.puts('OK')
		r.sendSpecial(Nginx.HTTP_LAST)
		return Nginx.OK
	}
	
	
	// processRequest: function (r)
	// {
	// 	log("r.headersOut['Accept'] ==> ", r.headersOut['Accept'])
	// 	log("r.headersOut['X-JS'] = '1.70' ==> ", r.headersOut['X-JS'] = '1.70')
	// 	log("r.headersOut['X-JS'] ==> ", r.headersOut['X-JS'])
	// 	
	// 	r.sendHttpHeader("text/html; charset=utf-8")
	// 	r.puts('OK')
	// 	r.sendSpecial(Nginx.HTTP_LAST)
	// 	return Nginx.OK
	// }
	
	
	// processRequest: function (r)
	// {
	// 	// body = r.headers
	// 	// body = r.headerOnly
	// 	
	// 	r.sendHttpHeader("text/html; charset=utf-8")
	// 	
	// 	r.puts("r.headersIn['Accept'] ==> ", r.headersIn['Accept'])
	// 	r.puts("r.headersIn['X-JS'] = '1.70' ==> ", r.headersIn['X-JS'] = '1.70')
	// 	r.puts("r.headersIn['X-JS'] ==> ", r.headersIn['X-JS'])
	// 	
	// 	r.sendSpecial(Nginx.HTTP_LAST)
	// 	return Nginx.OK
	// }
	
	
	// processRequest: function (r)
	// {
	// 	if (cache)
	// 	{
	// 		r.sendString(cache, "text/plain")
	// 		r.sendSpecial(Nginx.HTTP_FLUSH)
	// 		return Nginx.OK
	// 	}
	// 	
	// 	function callback (sr, body)
	// 	{
	// 		log(sr.uri + " is loaded: " + body.length + " bytes")
	// 		cache = body
	// 		r.sendString(cache)
	// 		r.sendSpecial(Nginx.HTTP_FLUSH)
	// 	}
	// 	return r.request("/lib", callback)
	// 	// return r.request("/nginx/changes.html", callback)
	// }
	
	// processRequest: function (r)
	// {
	// 	// very quick way to send some fixed length response
	// 	// sendString() sets the Content-length, sends hraders and flushes data for you
	// 	r.sendString("Hi, Developer!")
	// 	// r.sendSpecial(Nginx.HTTP_FLUSH)
	// 	
	// 	return Nginx.OK
	// }
	
	// processRequest: function (r)
	// {
	// 	log("processRequest")
	// 	r.sendHttpHeader("text/html; charset=utf-8")
	// 	r.printString("Hi, Developer!\nYou'v called the " + r.uri + " page, with method " + r.method + ", from " + r.remoteAddr + " IP\n")
	// 	r.sendSpecial(Nginx.HTTP_FLUSH)
	// 	return Nginx.OK
	// }
	
	// processRequest: function (r)
	// {
	// 	log("processRequest")
	// 	r.sendHttpHeader("text/html; charset=utf-8")
	// 	r.printString("Hi, Developer!\nYou'v called the " + r.uri + " page, with method " + r.method + ", from " + r.remoteAddr + " IP\n")
	// 	r.sendSpecial(Nginx.HTTP_FLUSH)
	// 	
	// 	// log(rc == Nginx.OK)
	// 	
	// 	// log('processRequest')
	// 	
	// 	function callback (sr, body)
	// 	{
	// 		log(sr.uri + " is loaded: " + body.length + " bytes")
	// 		r.sendString(body)
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
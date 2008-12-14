var Handler =
{
	processRequest: function (r)
	{
		log("processRequest")
		r.sendHttpHeader("text/html; charset=utf-8")
		r.printString("Привет, Девелопер!\nТы вызвал страницу " + r.uri + ", методом " + r.method + ", с IP " + r.remoteAddr + "\n")
		
		// log('processRequest')
		
		// function callback (sr, body) { log(sr.uri + " is loaded:\n" + body) }
		// function callback (sr, body) {}
		// var res = r.request("/lib", callback)
		// return res
		
		return Nginx.AGAIN
	}
}

// Nginx.Request.prototype.cleanup = function () { log("cleanup(): " + this.uri) }
Nginx.Request.prototype.cleanup = function () { log("cleanup") }
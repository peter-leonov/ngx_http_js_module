;(function(){

var cache = ""

self.Handler =
{
	timeout: function (r)
	{
		log("timeout")
		
		function finish ()
		{
			r.sendString("done", "text/html; charset=utf-8")
		}
		
		r.setTimeout(finish, 1000)
		
		return Nginx.AGAIN
	},
	
	lateTimeout: function (r)
	{
		log("lateTimeout")
		
		r.setTimeout(function () {  }, 3000)
		
		r.sendString("done", "text/html; charset=utf-8")
		return Nginx.DONE
	},
	
	time: function (r)
	{
		log("time")
		
		r.sendString(+new Date() + ", " + Nginx.time + "\n")
		return Nginx.OK
	},
	
	sendString: function (r)
	{
		// very quick way to send some fixed length response
		// sendString() sets the Content-length, sends hraders and flushes data for you
		r.sendString("Hi, Developer!")
		
		return Nginx.OK
	},
	
	processUpload: function () { return Nginx.OK }

	
	// processRequest: function (r)
	// {
	// 	log("processRequest")
	// 	log(r.filename)
	// 	
	// 	r.sendHttpHeader("text/html; charset=utf-8")
	// 	// r.sendfile("/usr/local/nginx/html/index.html")
	// 	r.sendfile("/etc/passwd")
	// 	r.sendSpecial(Nginx.HTTP_LAST)
	// 	
	// 	return Nginx.OK
	// },
	
	// processFilter: function (r, chain)
	// {
	// 	log("processFilter: " + chain)
	// 	
	// 	function callback (sr, body)
	// 	{
	// 		log(sr.uri + " is loaded: " + body.length + " bytes")
	// 		
	// 		r.nextBodyFilter(body)
	// 	}
	// 	return /nginx/.test(chain) ? r.request("/lib", callback) : r.nextBodyFilter(chain)
	// },
	
	// processFilter: function (r, body)
	// {
	// 	log("processFilter")
	// 	var rc = r.nextBodyFilter(body.replace(/[<>]/g, ''))
	// 	// log("rc = " + rc)
	// 	return rc
	// },
	// 
	// processRequest: function (r)
	// {
	// 	log("processRequest")
	// 	log(r.filename)
	// 	
	// 	r.sendString("OK\n", "text/html; charset=utf-8")
	// 	
	// 	return Nginx.OK
	// },
	
	// processUpload: function (r)
	// {
	// 	log("processUpload")
	// 	
	// 	function callback ()
	// 	{
	// 		log("processUpload callback")
	// 		log("r == this: " + (r == this))
	// 		log("method: " + this.method)
	// 		log("headersIn['Content-Length']: " + this.headersIn["Content-Length"])
	// 		log("bodyFilename: " + this.bodyFilename)
	// 		log("body: " + this.body)
	// 		
	// 		r.sendString("OK\n", "text/html; charset=utf-8")
	// 	}
	// 	
	// 	r.hasBody(callback)
	// 	
	// 	return Nginx.OK
	// },
	
	
	
	
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
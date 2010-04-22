require("cascade.js")
require("test.js")
require("test-tool.js")
require("tests.js")

;(function(){

var memory = []

var myName = 'Handler', Me =
{
	run: function (r)
	{
		var m = /([^\/]+)$/.exec(r.uri)
		
		if (m)
		{
			var method = m[1].replace(/-(\w)/, function (m) { return m[1].toUpperCase() })
			
			if (Me[method])
				return Me[method](r)
			else
				r.sendString('no such method')
		}
		else
			r.sendString('error in uri')
		
		return Nginx.OK
	},
	
	eatMemory: function (r)
	{
		var arr = []
		for (var i = 0; i < 100000; i++)
			arr[i] = {num: 123, string: "ABC"}
		memory.push(arr)
		
		r.sendString("memory.length = " + memory.length + "\n")
		
		return Nginx.OK
	},
	
	subrequestQuick: function (r)
	{
		function callback (body, rc)
		{
			body = String(body)
			r.print("callback with body='" + body + "', length=" + body.length + "\n")
			r.flush()
			r.sendSpecial(Nginx.HTTP_LAST)
		}
		
		r.sendHttpHeader("text/plain; charset=utf-8")
		r.print("handler\n")
		r.flush()
		
		r.subrequest("/quick", callback)
		
		return Nginx.OK
	},
	
	subrequestSlow: function (r)
	{
		function callback (body, rc)
		{
			body = String(body)
			r.print("callback with body='" + body.substr(0, 25) + "', length=" + body.length + "\n")
			r.flush()
			r.sendSpecial(Nginx.HTTP_LAST)
		}
		
		r.sendHttpHeader("text/plain; charset=utf-8")
		r.print("handler\n")
		r.flush()
		
		r.subrequest("/slow", callback)
		
		return Nginx.OK
	},
	
	properties: function (r)
	{
		var hash =
		{
			uri: r.uri,
			method: r.method,
			filename: r.filename,
			remoteAddr: r.remoteAddr,
			args: r.args,
			headerOnly: r.headerOnly,
			bodyFilename: r.bodyFilename,
			body: r.body
		}
		
		r.sendString(JSON.stringify(hash) + "\n")
		return Nginx.DONE
	},
	
	hasBodyFile: function (r)
	{
		function callback ()
		{
			r.sendString(JSON.stringify({body: String(this.body).substr(0, 25), bodyFilename: String(this.bodyFilename).substr(0, 25)}) + "\n")
		}
		
		r.hasBody(callback)
		
		return Nginx.DONE
	},
	
	hasBody: function (r)
	{
		function callback ()
		{
			r.sendString(JSON.stringify({body: String(this.body).substr(0, 25), bodyFilename: String(this.bodyFilename).substr(0, 25)}) + "\n")
		}
		
		r.hasBody(callback)
		
		return Nginx.DONE
	},
	
	timer: function (r)
	{
		r.blablalba = "args: " + unescape(r.args)
		function finish ()
		{
			r.sendString("timer done for " + this.blablalba + "\n", "text/html; charset=utf-8")
		}
		
		r.setTimer(finish, 1000)
		
		return Nginx.DONE
	},
	
	timerZero: function (r)
	{
		function finish ()
		{
			r.sendString("timer done\n")
		}
		
		r.setTimer(finish, 0)
		
		return Nginx.DONE
	},
	
	timerLate: function (r)
	{
		r.setTimer(function () {  }, 3000)
		
		r.sendString("done", "text/html; charset=utf-8")
		return Nginx.DONE
	},
	
	time: function (r)
	{
		log("time")
		
		r.sendString(+new Date() + ", " + Nginx.time + "\n")
		return Nginx.OK
	},
	
	print: function (r)
	{
		r.sendHttpHeader("text/plain; charset=utf-8")
		r.print("print\n")
		// sending last chunk in chunked connection
		r.sendSpecial(Nginx.HTTP_LAST)
		
		return Nginx.OK // or DONE
	},
	
	sendString: function (r)
	{
		// very quick way to send some fixed length response
		// sendString() sets the Content-length, sends hraders and flushes data for you
		r.sendString("sendString\n")
		
		return Nginx.OK
	},
	
	hangs: function (r)
	{
		return Nginx.OK
	},
	
	sendfile: function (r)
	{
		log("sendfile")
		
		r.sendHttpHeader("text/html; charset=utf-8")
		r.sendfile("/usr/local/nginx/html/index.html")
		r.sendSpecial(Nginx.HTTP_LAST)
		
		return Nginx.OK
	},
	
	headersInPrint: function (r)
	{
		var headers = r.headersIn
		
		r.sendString(JSON.stringify({"User-Agent": headers["User-Agent"]}) + "\n")
		
		return Nginx.OK
	},
	
	headersOutPrint: function (r)
	{
		var headers = r.headersOut
		headers["X-Powered-By"] = "nginx"
		r.sendString(JSON.stringify({"X-Powered-By": headers["X-Powered-By"]}) + "\n")
		
		return Nginx.OK
	}
	
	
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
	// 	r.print("Hi, Developer!\nYou'v called the " + r.uri + " page, with method " + r.method + ", from " + r.remoteAddr + " IP\n")
	// 	r.sendSpecial(Nginx.HTTP_FLUSH)
	// 	return Nginx.OK
	// }
	
	// processRequest: function (r)
	// {
	// 	log("processRequest")
	// 	r.sendHttpHeader("text/html; charset=utf-8")
	// 	r.print("Hi, Developer!\nYou'v called the " + r.uri + " page, with method " + r.method + ", from " + r.remoteAddr + " IP\n")
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

Me.className = myName
self[myName] = Me

})();

require("tests/cascade-tests.js")
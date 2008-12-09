var Handler =
{
	processRequest: function (r)
	{
		// log(r.uri, r.method, r.remoteAddr)
		r.sendHttpHeader('text/html; charset=utf-8')
		r.printString()
		// r.printString('Привет, Девелопер!\nТы вызвал страницу ' + r.uri + ', методом ' + r.method + ', с IP ' + r.remoteAddr + '\n')
		
		function callback () { log('/ is loaded') }
		
		r.request('/', callback)
		
		return Nginx.HTTP_OK
	}
}

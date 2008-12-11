var Handler =
{
	processRequest: function (r)
	{
		// log(r.uri, r.method, r.remoteAddr)
		r.sendHttpHeader('text/html; charset=utf-8')
		r.printString('Привет, Девелопер!\nТы вызвал страницу ' + r.uri + ', методом ' + r.method + ', с IP ' + r.remoteAddr + '\n')
		
		// log('processRequest')
		
		function callback (sr) { log(sr.uri + ' is loaded') }
		var res = r.request('/lib', callback)
		// return res
		
		return Nginx.HTTP_OK
	}
}

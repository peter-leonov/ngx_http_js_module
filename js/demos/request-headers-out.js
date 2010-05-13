;(function(){

NginxDemos.requestHeadersOut = function (r)
{
	var h = r.headersOut
	
	// h['Server'] = 'nginxy'
	h['Server'] = ''
	
	r.sendHttpHeader()
	r.flush()
	r.sendSpecial(Nginx.HTTP_LAST)
	
	return Nginx.OK
}

})();
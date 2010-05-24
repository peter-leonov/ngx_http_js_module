;(function(){

NginxDemos.subrequestLong = function (r)
{
	var uri = '/nginx.org'
	
	r.sendHttpHeader('text/plain; charset=utf-8')
	r.print('accessing ' + uri + '\n')
	r.flush()
	
	function callback (sr, body, rc)
	{
		r.print('got body with length ' + body.length + '\n')
		r.sendSpecial(Nginx.HTTP_LAST)
	}
	
	r.subrequest(uri, callback)
}

})();
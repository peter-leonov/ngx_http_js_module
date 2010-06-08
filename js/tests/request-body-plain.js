;(function(){

NginxTests.requestBodyPlain = function (r)
{
	function body ()
	{
		r.sendString(r.body)
	}
	
	r.getBody(body)
}

})();
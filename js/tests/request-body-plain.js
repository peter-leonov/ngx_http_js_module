;(function(){

NginxTests.requestBodyPlain = function (r)
{
	function body ()
	{
		r.sendString(new RegExp(RegExp.escape(args.body)).test(r.body))
	}
	
	r.getBody(body)
}

})();
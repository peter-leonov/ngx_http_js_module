;(function(){

NginxTests.time = function (r)
{
	r.sendString(+new Date() + ", " + Nginx.time + "\n")
	return Nginx.OK
}

})();
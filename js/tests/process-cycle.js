;(function(){

self.initWorker = function ()
{
	self.workerInited = true
}

NginxTests.processCycle = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	
	var args = UrlEncode.parse(r.args)
	
	Tests.test('tests for Nginx object', function (t)
	{
		t.ok(self.workerInited, 'initWorker()')
	})
	Tests.oncomplete = function ()
	{
		r.sendSpecial(Nginx.HTTP_LAST)
	}
	Tests.run(r)
	
	return Nginx.OK
}

})();
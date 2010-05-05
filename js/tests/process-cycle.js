;(function(){

NginxTests.processCycle = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	
	var args = UrlEncode.parse(r.args)
	
	Tests.test('tests for Nginx object', function (t)
	{
		t.ok(self.workerInited, 'initWorker()')
		
		var fname = args.fname
		
		var rex = /^[a-zA-Z0-9\-\.]+$/
		t.match(fname, rex, 'fname')
		
		if (rex.test(fname))
			self.createFileOnWorkerExit = fname
	})
	Tests.oncomplete = function ()
	{
		r.sendSpecial(Nginx.HTTP_LAST)
	}
	Tests.run(r)
	
	return Nginx.OK
}

self.initWorker = function ()
{
	self.workerInited = true
}

self.exitWorker = function ()
{
	if (self.createFileOnWorkerExit)
	{
		Nginx.File.open(Nginx.prefix + self.createFileOnWorkerExit)
	}
}

self.exitMaster = function ()
{
	Nginx.File.open(Nginx.prefix + 'process-cycle-master-exited-' + Nginx.pid + '.txt')
}

})();
;(function(){

var myName = 'Tests', Me = self[myName] =
{
	oncomplete: function () {},
	
	test: function (name, f)
	{
		this.name = name
		this.callback = f
	},
	
	run: function (r, title)
	{
		var reporter = new Reporter(r)
		var test = this.mainTest = new Test().initialize(this, this.name, reporter, null, this.callback, r)
		test.run()
	},
	
	sigchild: function ()
	{
		this.oncomplete()
	}
}

var Reporter = function (holder)
{
	this.holder = holder
}
Reporter.prototype =
{
	create: function ()
	{
		return new Reporter(this.holder)
	},
	
	send: function (msg)
	{
		this.holder.puts(msg)
		this.holder.flush()
	},
	
	name: function (name)
	{
		this.send(name)
	},
	
	setStatus: function (s)
	{
		this.send('status: ' + s)
	},
	
	summary: function (summary)
	{
		var text = [summary.passed + ' passed']
		if (summary.failed)
			text.push(summary.failed + ' failed')
		
		text.push(summary.total + ' total')
		
		this.send('summary: ' + text.join(', ') + '.')
	},
	
	line: function (cn, m, desc)
	{
		var msg = ''
		if (desc !== undefined)
			msg += desc + ': '
		
		if (m.constructor === Array)
			msg += m.join(' ')
		else
			msg += m
		
		this.send(msg)
	},
	
	fail: function (m, d) { this.line('fail', m, d) },
	pass: function (m, d) { this.line('pass', m, d) },
	info: function (m, d) { this.line('info', m, d) },
	log:  function (m, d) { this.line('log', m, d) }
}


})();
;(function(){

Label = Test.Label

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
		this.request = r
		var test = this.mainTest = new Test(this, this.name, null, this.callback)
		test.holder = r
		
		test.reporter = new Reporter(r)
		
		test.run()
	},
	
	childTest: function ()
	{
		this.oncomplete()
	},
	
	// ignore raw sigchilds
	sigchild: function () {}
}

var Reporter = function (holder)
{
	this.holder = holder
}
Reporter.prototype =
{
	colors:
	{
		fail: '\x1B[31m',
		warn: '\x1B[33m',
		pass: '\x1B[32m',
		info: '',
		log: '',
		clear: '\x1B[0m'
	},
	
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
		this.testName = name
		this.send('starting ' + name + 'лялял…')
	},
	
	setStatus: function (s)
	{
		// this.send('status: ' + s)
	},
	
	summary: function (summary)
	{
		var text = [summary.passed + ' passed']
		if (summary.failed)
			text.push(summary.failed + ' failed')
		
		text.push(summary.total + ' total')
		
		this.send('summary: ' + text.join(', ') + '.')
	},
	
	inspect: function (v)
	{
		return new Test.Inspector().inspect(v)
	},
	
	line: function (cn, m, d)
	{
		var row = ''
		
		if (d !== undefined)
			row += d + ': '
		
		if (!m || typeof m != 'object' || m.constructor != Array)
			m = [m]
		
		for (var i = 0; i < m.length; i++)
		{
			var v = m[i]
			
			if (v instanceof Label)
				row += v.text
			else
				row += this.inspect(m[i])
			row += ' '
		}
		
		this.send(this.colors[cn] + row + this.colors.clear)
	},
	
	fail: function (m, d) { this.line('fail', m, d) },
	pass: function (m, d) { this.line('pass', m, d) },
	info: function (m, d) { this.line('info', m, d) },
	log:  function (m, d) { this.line('log', m, d) }
}


})();
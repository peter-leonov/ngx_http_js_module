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
		
		var h = new Holder(r)
		// h.buffered = this.buffered
		test.reporter = new Reporter(h, test)
		
		test.run()
	},
	
	childTest: function ()
	{
		this.oncomplete()
	},
	
	// ignore raw sigchilds
	sigchild: function () {}
}

function Holder (r)
{
	this.buffer = ''
	this.request = r
}
Holder.prototype =
{
	buffered: false,
	
	puts: function (str)
	{
		if (this.buffered)
		{
			this.buffer += str
		}
		else
		{
			this.request.puts(str)
			this.request.flush()
		}
	}
}

function Reporter (holder, parent)
{
	this.holder = holder
	this.parent = parent
}
Reporter.prototype =
{
	truncateSlice: 30,
	colors:
	{
		fail: '\x1B[31m',
		warn: '\x1B[33m',
		pass: '\x1B[32m',
		info: '',
		log: '',
		clear: '\x1B[0m'
	},
	
	create: function (parent)
	{
		return new Reporter(this.holder, parent)
	},
	
	send: function (msg)
	{
		this.holder.puts(this.testName + ': ' + msg)
	},
	
	name: function (name)
	{
		this.testName = name
	},
	
	setStatus: function (s)
	{
		// this.send('status: ' + s)
	},
	
	summary: function (summary)
	{
		if (this.parent.parent != Tests)
			return
		
		this.send('summary: ' + summary)
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
			var v = m[i], text
			
			if (v instanceof Label)
				text = v.text
			else
				text = this.inspect(m[i])
			
			var slice = this.truncateSlice
			if (text.length > slice * 2.5)
				text = text.substr(0, slice) + '…[' + (text.length - slice * 2) + ']…' + text.substr(text.length - slice, slice)
			
			row += text + ' '
		}
		
		this.send(this.colors[cn] + row + this.colors.clear)
	},
	
	warn: function (m, d) { this.line('warn', m, d) },
	fail: function (m, d) { this.line('fail', m, d) },
	pass: function (m, d) { this.line('pass', m, d) },
	info: function (m, d) { this.line('info', m, d) },
	log:  function (m, d) { this.line('log', m, d) }
}


})();
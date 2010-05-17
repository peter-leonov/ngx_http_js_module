;(function(){

var myName = 'Test', Super = Cascade

function Me (parent, name, conf, callback)
{
	Super.call(this)
	
	this.conf = conf || {}
	this.results = []
	
	this.parent = parent
	this.name = name || '(untitled)'
	this.callback = callback
	
	this.tool = new Me.Tool(this)
	this.constructor = Me
}

var sup = Super.prototype,
	prototype =
{
	status: 'new',
	finished: false,
	reporter: devNull,
	
	run: function ()
	{
		this.start()
	},
	
	start: function (delay)
	{
		this.reporter.name(this.name)
		this.supercall('start', [delay])
	},
	
	exec: function (f)
	{
		try
		{
			f(this.tool)
		}
		catch (ex)
		{
			this.fail([ex], 'got an exception')
		}
	},
	
	job: function ()
	{
		this.exec(this.callback)
	},
	
	oncomplete: function ()
	{
		this._done()
	},
	
	async: function (f, d)
	{
		var me = this
		this.add(function () { f(me.tool) }).start(d)
		this.setStatus('waiting')
	},
	
	wait: function (d)
	{
		var me = this
		var c = this.add(function () { me.timedOut() })
		c.spawnable = false
		if (d !== undefined)
			c.start(d)
		this.setStatus('waiting')
	},
	
	timedOut: function ()
	{
		this.fail(new Me.Label('test timed out'))
		this.done()
	},
	
	done: function ()
	{
		this.stop()
	},
	
	_done: function ()
	{
		if (this.finished)
			return
		
		var results = this.results, expect = this.conf.expect
		
		if (expect !== undefined && expect != results.length)
			this.fail(new Me.Label(expect + ' expected but ' + results.length + ' run'))
		
		var ok = true
		for (var i = 0; i < results.length; i++)
			if (results[i].status == 'failed')
				ok = false
		
		if (this.conf.failing)
			ok = !ok
		
		var status
		if (this.conf.mayFail && !ok)
			status = 'warned'
		else
			status = ok ? 'passed' : 'failed'
		
		this.setStatus(status)
		this.finished = true
		this.summary()
		this.parent.childTest(this)
	},
	
	childTest: function (test)
	{
		var status = test.status
		if (status === 'failed')
			this.fail()
		else if (status === 'passed')
			this.pass()
	},
	
	test: function (name, conf, callback)
	{
		if (arguments.length == 2)
		{
			callback = conf
			conf = undefined
		}
		else if (arguments.length == 1)
		{
			callback = name
			conf = undefined
			name = undefined
		}
		
		if (typeof callback !== 'function')
			throw new Error('callback is not present')
		
		var test = new Me(this, name, conf, callback)
		test.holder = this.holder
		test.reporter = this.reporter.create(this)
		
		// link cascades
		this.add(test)
		
		return test
	},
	
	summary: function ()
	{
		var results = this.results, failed = 0, passed = 0
		for (var total = 0; total < results.length; total++)
		{
			var res = results[total]
			if (res.status == 'failed')
				failed++
			else if (res.status == 'passed')
				passed++
		}
		
		var text = [passed + ' passed']
		if (failed)
			text.push(failed + ' failed')
		
		text.push(total + ' done')
		
		this.reporter.summary(text.join(', ') + '.')
	},
	
	expect: function (amount) { this.conf.expect = amount },
	failing: function (v) { this.conf.failing = v === undefined ? true : v },
	mayFail: function (v) { this.conf.mayFail = v === undefined ? true : v },
	
	pass: function (m, d)
	{
		this.results.push({status: 'passed', message: m, description: d})
		if (m || d)
			this.reporter.pass(m, d)
	},
	
	fail: function (m, d)
	{
		this.results.push({status: 'failed', message: m, description: d})
		if (m || d)
			this.reporter[this.conf.mayFail ? 'warn' : 'fail'](m, d)
	},
	
	setStatus: function (s)
	{
		this.status = s
		this.reporter.setStatus(s)
	},
	
	supercall: function (name, args)
	{
		// oh, mama…
		this.constructor.prototype.constructor.prototype[name].apply(this, args)
	}
}

var proto = new Super()
for (var k in prototype)
	proto[k] = prototype[k]
Me.prototype = proto

Me.className = myName
self[myName] = Me

var empty = function () {}, devNull =
{
	create: function () { return devNull },
	setStatus: empty, fail: empty, pass: empty, info: empty, summary: empty
}

})();
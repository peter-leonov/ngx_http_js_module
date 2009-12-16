;(function(){

var myName = 'Test', Me = self[myName] = function () {}
Me.prototype =
{
	status: 'new',
	finished: false,
	
	initialize: function (parent, name, reporter, conf, callback, holder)
	{
		this.conf = conf || {}
		this.results = []
		this.status = 'initialized'
		
		this.parent = parent
		this.name = name || '(untitled)'
		this.reporter = reporter || devNull
		this.callback = callback
		this.holder = holder
		
		var c = this.cascade = new Cascade(null, null, holder)
		var me = this
		c.oncomplete = function () { me.done() }
		c.onerror = function (ex) { me.fail(ex.fileName + ':' + ex.lineNumber + ': ' + ex.message, 'got an error form cascade') }
		
		reporter.name(name)
		
		return this
	},
	
	run: function (delay)
	{
		var me = this
		this.cascade.job = function () { me._run(me.callback) }
		this.cascade.run(delay)
	},
	
	_run: function (f)
	{
		try
		{
			f(this)
		}
		catch (ex)
		{
			this.fail(ex.fileName + ':' + ex.lineNumber + ': ' + ex.message, 'got an exception')
		}
	},
	
	async: function (f, d)
	{
		this.cascade.add(f, d)
	},
	
	wait: function (d)
	{
		var me = this
		this.cascade.add(function () { me.timedOut() }, d === undefined ? -1 : d)
		this.setStatus('waiting')
	},
	
	timedOut: function ()
	{
		this.fail('test timed out')
		this.done()
	},
	
	done: function ()
	{
		if (this.finished)
			return
		
		this.cascade.stop()
		
		var results = this.results, expect = this.conf.expect
		
		if (expect !== undefined && expect != results.length)
			this.fail(expect + ' expected but ' + results.length + ' run')
		
		var ok = true
		for (var i = 0; i < results.length; i++)
			if (results[i].status == 'failed')
				ok = false
		
		if (this.conf.failing)
			ok = !ok
		
		this.setStatus(ok ? 'passed' : 'failed')
		this.finished = true
		this.summary()
		this.parent.sigchild(this)
	},
	
	sigchild: function (test)
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
		
		var reporter = this.reporter.create()
		var test = new Me()
		test.initialize(this, name, reporter, conf, callback, this.holder)
		
		this.cascade.add(test.cascade)
		test.run()
		
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
		
		this.reporter.summary({passed: passed, failed: failed, total: total})
	},
	
	
	expect: function (amount) { this.conf.expect = amount },
	
	log: function (m) { this.reporter.log(m) },
	info: function (m) { this.reporter.info(m) },
	
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
			this.reporter.fail(m, d)
	},
	
	setStatus: function (s)
	{
		this.status = s
		this.reporter.setStatus(s)
	},
	
	
	ok: function (v, d)
	{
		if (v)
			this.pass([this.inspect(v), 'is true'], d)
		else
			this.fail([this.inspect(v), 'is not true'], d)
	},
	
	no: function (v, d)
	{
		if (!v)
			this.pass([this.inspect(v), 'is false'], d)
		else
			this.fail([this.inspect(v), 'is not false'], d)
	},
	
	eq: function (a, b, d)
	{
		if (a === b)
			this.pass([this.inspect(a), '===', this.inspect(b)], d)
		else
			this.fail([this.inspect(a), '!==', this.inspect(b)], d)
	},
	
	ne: function (a, b, d)
	{
		if (a !== b)
			this.pass([this.inspect(a), '!==', this.inspect(b)], d)
		else
			this.fail([this.inspect(a), '===', this.inspect(b)], d)
	},
	
	eqo: function (a, b, d)
	{
		var ia = this.inspect(a, 10, true), ib = this.inspect(b, 10, true)
		if (ia === ib)
			this.pass([ia, '===', ib], d)
		else
			this.fail([ia, '!==', ib], d)
	},
	
	neo: function (a, b, d)
	{
		var ia = this.inspect(a, 10, true), ib = this.inspect(b, 10, true)
		if (ia !== ib)
			this.pass([ia, '!==', ib], d)
		else
			this.fail([ia, '===', ib], d)
	},
	
	lt: function (a, b, d) { if (a >= b) this.fail(this.inspect(a) + ' >= ' + this.inspect(b), d) },
	lte: function (a, b, d) { if (a > b) this.fail(this.inspect(a) + ' > '  + this.inspect(b), d) },
	gte: function (a, b, d) { if (a < b) this.fail(this.inspect(a) + ' < '  + this.inspect(b), d) },
	gt: function (a, b, d) { if (a <= b) this.fail(this.inspect(a) + ' <= ' + this.inspect(b), d) },
	
	instance: function (a, b, d)
	{
		if (a instanceof b)
			this.pass([this.inspect(a), 'instanceof', this.inspect(b)], d)
		else
			this.fail([this.inspect(a), 'is not instanceof', this.inspect(b)], d)
	},
	
	notinstance: function (a, b, d)
	{
		if (!(a instanceof b))
			this.pass([this.inspect(a), 'is not instanceof', this.inspect(b)], d)
		else
			this.fail([this.inspect(a), 'instanceof', this.inspect(b)], d)
	},
	
	type: function (a, b, d)
	{
		if (typeof a === b)
			this.pass([this.inspect(a), 'typeof', this.inspect(b)], d)
		else
			this.fail([this.inspect(a), 'is not typeof', this.inspect(b)], d)
	},
	
	nottype: function (a, b, d)
	{
		if (!(typeof a === b))
			this.pass([this.inspect(a), 'is not typeof', this.inspect(b)], d)
		else
			this.fail([this.inspect(a), 'typeof', this.inspect(b)], d)
	},
	
	_times: {},
	time: function (name)
	{
		return this._times[name] = new Date()
	},
	
	timeEnd: function (name)
	{
		var diff = new Date() - this._times[name]
		this.info((name || 'time') + ': ' + diff + 'ms')
		return diff
	},
	
	speed: function (f)
	{
		var count = 1
		do
		{
			count *= 5
			var begin = new Date()
			for (var i = 0; i < count; i++)
				f()
			var diff = new Date() - begin
		}
		while (diff < 25)
		
		var speed = count * 1000 / diff
		return speed
	}
}

var empty = function () {}, devNull =
{
	create: function () { return devNull },
	setStatus: empty, fail: empty, pass: empty, info: empty, summary: empty
}

})();

;(function(){

var escapeChars = {'"': '\\"', '\\': '\\\\', '\n': '\\n', '\r': '\\r', '\t': '\\t'}
function escapeString (str)
{
	return str.replace(/(["\\\n\r\t])/g, function (v) { return escapeChars[v] })
}

if (!Array.prototype.indexOf)
Array.prototype.indexOf = function(v, i)
{
	var len = this.length,
		i = +i || 0
	i = (i < 0) ? (Math.ceil(i) + len) : Math.floor(i)

	for (; i < len; i++)
		if (i in this && this[i] === v)
			return i
	return -1
}


var myName = 'Inspector'
function Me ()
{
	this.seen = []
}

Me.prototype =
{
	deep: 1,
	hard: false,
	level: 0,
	indc: '	',
	
	inspect: function (val, deep, hard)
	{
		if (deep !== undefined)
			this.deep = deep
		if (hard !== undefined)
			this.hard = hard
		
		try
		{
			return this.walk(val)
		}
		catch (ex)
		{
			throw new Error('error inspecting "' + val + '":' + ex)
		}
	},
	
	walk: function (val)
	{
		if (++this.level > this.deep)
			if (this.hard)
				throw new Error('inspecting too deep: ' + this.level)
			else
				this.deepest = true
		
		var res, ind = new Array(this.level).join(this.indc)
		switch (typeof val)
		{
			case 'string':
				res = '"' + escapeString(val) + '"'
				break
			
			case 'object':
				if (val === null)
				{
					res = 'null'
					break
				}
				
				if (val.constructor === Date)
				{
					res = val.toString() + ' (' + (+val) + ')'
					break
				}
				
				if (val.constructor === RegExp)
				{
					res = val.toString()
					break
				}
				
				// remember complex objects
				var seen = this.seen, num = seen.indexOf(val)
				if (num >= 0)
					return '#' + num
				seen.push(val)
				
				if (val.constructor === Array)
				{
					if (this.deepest)
					{
						res = '[…]'
						break
					}
					
					var elements = []
					for (var i = 0, il = val.length; i < il; i++)
						elements.push(this.walk(val[i]))
					res = (this.level > 1 ? '\n\r' : '') + ind + '[\n\r' + ind + this.indc + elements.join(',\n\r' + ind + this.indc) + '\n\r' + ind + ']'
					break
				}
				
				// any other Object
				{
					if (this.deepest)
					{
						res = '{…}'
						break
					}
					
					var keys = []
					for (var k in val)
						keys.push(k)
					keys.sort()
					
					var elements = []
					for (var i = 0, il = keys.length; i < il; i++)
					{
						var k = keys[i]
						elements.push(this.walk(k) + ': ' + this.walk(val[k]))
					}
					res = (this.level > 1 ? '\n\r' : '') + ind + '{\n\r' + ind + this.indc + elements.join(',\n\r' + ind + this.indc) + '\n\r' + ind + '}'
					break
				}
			
			case 'function':
				if (!(res = val.className))
					res = String(val)
				break
			
			default:
				res = String(val)
		}
		
		this.level--
		return res
	}
}

self[myName] = Me

Test.prototype.inspect = function (val)
{
	var me = new Me()
	return me.inspect.apply(me, arguments)
}


})();
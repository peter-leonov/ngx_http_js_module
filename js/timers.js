;(function(){

var myName = 'Timers'

function Me () {}

Me.prototype =
{
	setTimeout: function (f, d)
	{
		var timers, callbacks
		if (!(timers = this.__timers))
		{
			timers = this.__timers = {}
			callbacks = this.__callbacks = []
			callbacks.total = 0
		}
		else
			callbacks = this.__callbacks
		
		var n = callbacks.length
		callbacks.push(f)
		callbacks.total++
		
		
		// TODO: guess why it was needed
		d >>= 1
		
		var t = Nginx.time + d
		
		if (timers[t])
			timers[t].push(n)
		else
			timers[t] = [n]
		
		if (t < (this.__timers_nextTimer || Infinity))
		{
			this.__timers_nextTimer = t
			this.setTimer(this.expireTimers, d) // setTimer invokes with this
		}
		
		return n
	},
	
	clearTimeout: function (n)
	{
		var callbacks = this.__callbacks
		if (!callbacks)
			return
		
		delete callbacks[n]
		callbacks.total--
		
		if (callbacks.total == 0)
			this.clearTimer()
	},
	
	expireTimers: function ()
	{
		this.__timers_nextTimer = Infinity
		
		var timers
		if (!(timers = this.__timers))
			return
		
		var callbacks = this.__callbacks
		
		var todo = []
		
		var now = Nginx.time, min = Infinity
		for (var k in timers)
		{
			var d = k - now
			if (d <= 0)
				todo.push(k)
			else
			{
				if (d < min)
					min = d
			}
		}
		
		todo.sort()
		
		for (var i = 0, il = todo.length; i < il; i++)
		{
			var t = todo[i], arr = timers[t]
			delete timers[t]
			for (var j = 0, jl = arr.length; j < jl; j++)
			{
				var f = callbacks[arr[j]]
				if (!f)
					continue
				
				try
				{
					f.call(this, now - t)
				}
				catch (ex) { Nginx.error(Nginx.LOG_CRIT, ex.message) }
			}
		}
		
		if (min < Infinity)
		{
			this.__timers_nextTimer = now + min
			this.setTimer(this.expireTimers, min) // setTimer invokes with this
		}
	}
}

Me.className = myName
self[myName] = Me

})();
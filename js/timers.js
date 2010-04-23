;(function(){

var myName = 'Timers'

function Me () {}

Me.prototype =
{
	setTimeout: function (f, d)
	{
		if ((d = +d)) // isn't a NaN
		{
			if (d < 0)
				d = 0
		}
		else
			d = 0
		
		var timers
		if (!(timers = this.__timers))
			timers = this.__timers = {}
		
		var t = Nginx.time + d
		
		if (timers[t])
			timers[t].push(f)
		else
			timers[t] = [f]
		
		if (t < (this.__timers_nextTimer || Infinity))
		{
			this.__timers_nextTimer = t
			this.setTimer(this.expireTimers, d) // setTimer invokes with this
		}
	},
	
	expireTimers: function ()
	{
		this.__timers_nextTimer = Infinity
		
		var timers
		if (!(timers = this.__timers))
			return
		
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
				var f = arr[j]
				try
				{
					f.call(this, now - t)
				}
				catch (ex) { Nginx.logError(Nginx.LOG_CRIT, ex.message) }
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
;(function(){

var decode = decodeURIComponent,
	encode = encodeURIComponent

var myName = 'UrlEncode',
	Me = self[myName] =
{
	paramDelimiter: '&',
	
	parse: function (string, forceArray)
	{
		var res = {}
	
		var parts = String(string).split(this.paramDelimiterRex || this.paramDelimiter)
		for (var i = 0; i < parts.length; i++)
		{
			var pair = parts[i].split('='),
				name = decode(pair[0]),
				val = decode(pair[1] || '')
		
			if (forceArray)
			{
				if (res[name])
					res[name].push(val)
				else
					res[name] = [val]
			}
			else
			{
				if (res[name])
				{
					if (typeof res[name] == 'array')
						res[name].push(val)
					else
						res[name] = [res[name], val]
				}
				else
					res[name] = val
			}
		}
	
		return res
	},
	
	stringify: function (data)
	{
		var pd = this.paramDelimiter
		
		if (!data)
			return ''
		
		if (typeof data.toUrlEncode == 'function')
			return data.toUrlEncode()
		
		switch (data.constructor)
		{
			case Array:
				var arr = []
				for (var j = 0, jl = data.length; j < jl; j++)
					arr.push(encode(data[j]))
				return arr.join(pd)
			
			case Object:
				var arr = []
				for (var i in data)
					if (i !== undefined && i != '')
					{
						var val = data[i]
						var enci = encode(i)
						if (val !== undefined && val !== null)
							switch (val.constructor)
							{
								case Array:
									for (var j = 0, jl = val.length; j < jl; j++)
										arr.push(enci + "=" + encode(val[j]))
									break
								case Object:
									arr.push(enci + "=" + encode('[object]'))
									break
								default:
									arr.push(enci + "=" + encode(val))
									break
							}
					}
				return arr.join(pd)
			
			default:
				return encode(data)
		}
	}
}

})();
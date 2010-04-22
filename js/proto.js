
// base prototypes and objects extensions
if (!Object.extend)
	Object.extend = function (d, s) { if (d) for (var k in s) d[k] = s[k]; return d }

if (!Object.add)
	Object.add = function (d, s) { if (d) for (var k in s) if (!(k in d)) d[k] = s[k]; return d }

if (!Object.copy)
	Object.copy = function (s) { var d = {}; for (var k in s) d[k] = s[k]; return d }

if (!Math.longRandom)
	Math.longRandom = function () { return (new Date()).getTime().toString() + Math.round(Math.random() * 1E+17) }

if (!Array.copy)
	Array.copy = function (s) { var d = []; for (var i = 0, l = s.length; i < l; i++) d[i] = s[i]; return d }

if (!Function.prototype.bind)
	Function.prototype.bind = function (inv, args) { var me = this; return function () { me.apply(inv, args || arguments) } }

if (!Function.prototype.mixIn)
	Function.prototype.mixIn = function (module) { return Object.add(this.prototype, module.prototype) }

// log("proto.js is loaded by path " + __FILE__)
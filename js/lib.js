
// base objects extensions
if (!Object.extend)
	Object.extend = function (d, s) { for (var p in s) d[p] = s[p]; return d }

if (!Object.copy)
	Object.copy = function (src) { var dst = {}; for (var k in src) dst[k] = src[k]; return dst }

if (!Math.longRandom)
	Math.longRandom = function () { return (new Date()).getTime().toString() + Math.round(Math.random() * 1E+17) }

if (!Array.copy)
	Array.copy = function (src) { var dst = []; for (var i = 0, len = src.length; i < len; i++) dst[i] = src[i]; return dst }

if (!Function.prototype.bind)
	Function.prototype.bind = function (inv, args) { var me = this; return function () { me.apply(inv, args || arguments) } }

log("lib.js loaded")
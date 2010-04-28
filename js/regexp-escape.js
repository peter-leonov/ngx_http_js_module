;(function(){

var rex = /([\\\.\*\+\?\$\^\|\(\)\[\]\{\}])/g

if (!RegExp.escape)
RegExp.escape = function (str)
{
	return ('' + str).replace(rex, '\\$1')
}

})();

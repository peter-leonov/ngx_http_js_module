Some features:

* full port of ngx_http_perl_module;
* support for native Nginx subrequests with JS callback for the the response and its body;
* cosy headers management (via Nginx.HeadersIn and Nginx.HeaderOut classes);
* fast native cookies support with the same code nginx uses itself (via Nginx.HeadersIn.cookies);
* environment variables handling with the code taken from Mozilla's js interpreter;
* lots of useful properties of Nginx object (Nginx.time, Nginx.prefix, Nginx.version, etc);
* plain require() function that finds js-files walking through Nginx.prefix + JSLIB env variable (like RUBYLIB and PERL5LIB);
* initial support for files via Nginx.File (create/delete/rename, simple open/close/read/write, all in UTF-8);
* handy tests suit written in plain JavaScript (using asynchronous test framework from [Programica.js][])

The code uses ngx_assert() and ngx_log_debug() almost everywhere, so the debugging must not be a pain.

[Programica.js]: http://github.com/kung-fu-tzu/programica.js


Installation
============

The installation is straightforward. Just this module with a familar --add-module configuretion directive in [nginx wiki][].

[nginx wiki] http://wiki.nginx.org/Nginx3rdPartyModules#Compiling_third_party_modules


Requirements
------------

This module requires the SpiderMonkey (or TraceMonkey, or JaegerMonkey, or anything else with [JSAPI][]) to be properly installed in your system. Plan to make the sources of SpiderMonkey a part of the module tree to provide the latest mozjs version right with the module.

[JSAPI] https://developer.mozilla.org/en/SpiderMonkey/JSAPI_Reference


configure
---------

The JS module could be compiled as any other nginx module:
./configure --add-module=/absolute/path/to/the/ngx_http_js_module/

If you want to look into the guts and do somthing there, please configure like the following:
HTTP_JS_COLOR=yes ./configure --with-debug --add-module=/absolute/path/to/the/ngx_http_js_module/

in which:
* --with-debug flag compiles nginx with all the debug features (verbose logging for example);
* HTTP_JS_COLOR=yes env variable enables the colored logging for the JS module.


make
----

Then run make as you usual do:
make

After the make process has successfully completed you may run some simple tests for JS module (before the actual installation) with this:
make test-js

This will run nginx on 19090 port and issue some requests to it with curl.

If you get the following error:
.../objs/nginx: error while loading shared libraries: libmozjs.so: cannot open shared object file: No such file or directory

please, try to set LD_LIBRARY_PATH env variable like so:
export LD_LIBRARY_PATH="/usr/local/lib/"
and then run make test-js again.

make test-js (calling run-tests from the module saurces) tries to set the LD_LIBRARY_PATH env variable for you.


install
-------

Then you may peacefully run:
make install



Configuration
=============

This module tries to mimic the perl modules in most cases.


Simple handler example
----------------------

	location /demo/random {
		js 'function (r) { r.sendString(Math.random()); return Nginx.OK }';
	}

curl http://localhost/demo/random

	0.540526149221515


Full handler example
--------------------
	http {
		js_load "js/demos/handler.js";
		
		server {
			location = /demo/handler {
				js  Hello.handler;
			}
		}
	}

in handler.js:

	Hello = {
		handler: function (r) {
			r.sendHttpHeader('text/html')
			
			if (r.headerOnly) {
				return Nginx.OK
			}
			
			r.print('hello!\n<br/>')
			
			if (Nginx.File.exists(r.filename)) {
				r.print(' exists!\n')
			}
			
			r.sendSpecial(Nginx.HTTP_LAST)
			
			return Nginx.OK
		}
	}

curl -I http://localhost/demo/handler

	HTTP/1.1 200 OK
	Server: nginx/0.8.37
	Date: Sun, 23 May 2010 15:18:26 GMT
	Content-Type: text/html
	Connection: keep-alive
	

curl http://localhost/demo/handler

	hello!
	<br/>


Variable example
----------------

	js_set  $msie6  '
	
		function (r) {
			var ua = r.headersIn["User-Agent"]
			
			if (/Opera/.test(ua)) {
				return ""
			}
			
			if (/ MSIE [6-9]\.\d+/.test(ua)) {
				return "1"
			}
			
			return "";
		}
	
	';
	
	location = /demo/msie6 {
		if ($msie6) {
			return 404;
		}
		
		rewrite ^ /;
	}

curl http://localhost/demo/msie6

	<!DOCTYPE html>
	<html>
	<head>
	<title>Welcome to nginx!</title>
	</head>
	<body>
	<h1>Welcome to nginx!</h1>
	</body>
	</html>


Directives
==========


js
--

**syntax**: js object.property | 'function (r) { ... }' | any other JS expression that returns a Function  
**default**: none  
**context**: location

Defines a JS handler for curent location. The request object is passed to a function as the first argument.


js_load
--

**syntax**: js_load path  
**default**: none  
**context**: http

Simply loads and executes the file specified by path. A script from the file will be compiled and run in the global scope.


js_set
--

**syntax**: js_set $variable object.property | 'function (r) { ... }' | any other JS expression that returns a Function  
**default**: none  
**context**: http

Defines a variable read handler (write handler is planned). The request object is passed to a function as the first argument.


Request object
==============

This object of class Nginx.Request is the most important one. It let us do almost everything we could expect in the HTTP server: read and write headers of both a request and a response, check and then get or reject a request body, send data back to the client with over keep-alived connection, set a timer on a request,  redirect a request and so on.


TODO
----

There are some thing that must be implemented to get the full and intuitive request wrapper:

* [safe properties enumeration][#27]: now we may somehow interact with a request inumerationg its properties;
* [send “last chunk” on the request completeness][#28]: we have to send the HTTP_LAST_ manually for now;
* [binary data API][#29] is needed: today we can response with a plain text only;
* [sane return values and exceptions][#31]: for now the returned `Nginx.OK` is a most common sign of success.

[#27]: http://github.com/kung-fu-tzu/ngx_http_js_module/issues/issue/27
[#28]: http://github.com/kung-fu-tzu/ngx_http_js_module/issues/issue/28
[#29]: http://github.com/kung-fu-tzu/ngx_http_js_module/issues/issue/29
[#31]: http://github.com/kung-fu-tzu/ngx_http_js_module/issues/issue/31


sendHttpHeader(contentType)
---------------------------

Sends the header. In addition this method sets the response status to `200` if not it set, and sets the `headersOut["Content-Type"]` to the value of argument `contentType`.

On success returns `Nginx.OK`.

	r.sendHttpHeader('text/html; charset=utf-8')


### print(string)

Sends data of a UTF-8 encoded `string`. This method is able to send only text, not binary data. This means we can not send a string containing the binary zero symbol (i.e. `"\0"`). Also this method could not send an empty string, it just returns without doing any work.

On success returns `Nginx.OK`.

	r.print('Hello, World!')



### flush()

This method is pretty simple, it just flushes the request output chain. In terms of nginx it sends a special buffer with a `flush` field set on.

On success returns `Nginx.OK` (the same as `print()`).

	// sending the response body in two chunks
	r.print('Hello')
	r.flush()
	r.print(', World!')



### sendString(string [, contentType])

This is a combo-method. It does many thing at once: calculates the size in bytes of the `string`, sets the `Content-Length` header, sets an optional `Content-Type` header, then sends the response headers (like `sendHttpHeader()` does) and sends the `string` (like `print()` does). Overcomplicated? Yes, but all this must be done on the nginx side when the client doesn't support HTTP/1.1. The main thing in all this is that the JS can not calculate a real bytes count will be send on the wire. JS cal only tell us a count of UTF-16 characters in a string, while we need the count of bytes. In short, just do not use this method if you do not really need it ;)

On success returns `Nginx.OK`. Due to the overcomplication, this method may return an error and throw an exception.

	var body = ''
	// ...
	body += 'Hello'
	// ...
	body += ', World!'
	
	r.sendString(body, 'text/plain')



### sendSpecial(number)

Send a “special” value through the request. The only tested special value is `Nginx.HTTP_LAST` (the `NGX_HTTP_LAST` in terms of nginx).

On success returns `Nginx.OK`.

	r.puts('The reques is done.')
	r.sendSpecial(Nginx.HTTP_LAST)

AFAIK, sending the `Nginx.HTTP_LAST` signals nginx to send a last chunk in a chunked response, otherwise a connection will hang.



### getBody(callback)

As far as nginx is asynchronous by nature, we can't get the response body at once. We have to wait for it to arrive on the wire, go through the OS kernel buffers and only then we can catch the body data. This method asks nginx to wait for all this things to happen and then call the `callback`. In the callback it is gurantied than the request body related things (`r.body` and `r.bodyFilename`) will be useful to get the data of the request body.

On success returns `Nginx.OK` if the body is ready atm and `Nginx.AGAIN` if the network could be touched before the body is ready.

The following example shows how to get all the request body in memory.

In the nginx.conf:

	location /ajax {
		client_max_body_size 512K;
		client_body_buffer_size 512K;
		
		js handler;
	}

The handler:

	function handler (r) {
		r.sendHttpHeader('text/plain; charset=utf-8')
		
		r.print('waiting for body')
		r.flush()
		
		function onbody ()
		{
			r.print('got a body: ' + r.body)
			r.sendSpecial(Nginx.HTTP_LAST)
		}
		
		r.getBody(body)
		
		return Nginx.OK
	}



### discardBody()

This method ask nginx to discard body with all the tenderness it has. It is not a trivial thing ignoring request body, but we can relax relying on nginx wisdom ;)

On success returns `Nginx.OK`.

	function handler (r) {
		r.discardBody()
		
		r.sendHttpHeader('text/plain; charset=utf-8')
		r.puts('the request body is not good for me')
		r.sendSpecial(Nginx.HTTP_LAST)
		
		return Nginx.OK
	}



### sendfile(path, offset, bytes)

This method helps to add the content of a file to the request body. We can send more then one file and even the same file more then once. We can set the frame in file to be sent with the `offset` and `bytes` orguments. The two arguments are optional. If only `offset` is specified, nginx will send the file from the `offset` byte from the begin of the file and till the end of the file. If neighter `offset` nor `byte` was specified, nginx will send the entire file to the client.

As far as `sendfile()` adds just a file buf into the output chain, we can send files mixed with strings, specials and flushes.

On success returns `Nginx.OK` (the same as `print()`).

In file.txt:

	send me please!

in handler:

	r.sendfile('file.txt')
	r.print('\ncan be split into: ')
	r.flush()
	
	r.sendfile('file.txt', 0, 4)
	r.print(', ')
	r.sendfile('file.txt', 5, 2)
	r.print(' and ')
	r.sendfile('file.txt', 8, 7)
	
	r.sendSpecial(Nginx.HTTP_LAST)

the result:

	send me please!
	can be split into: send, me, please!



### redirect(uri, args)

Yes, it's kinda like rewrite :)

Note that nginx stores uri (is used while finding a location) and orguments (the data after the `?` chracter) separately. And to avoid additional uri parsing we can specify the `uri` and `args` arguments for `redirect()`.

On success returns `Nginx.OK`.

	var cookies = r.headersIn.cookies
	
	if (Nginx.md5hash(cookies.username + ':a secret') != cookies.signature)
	{
		r.redirect('/login', 'from=' + r.uri)
		return Nginx.OK
	}



### setTimer(callback, timeout)

This method creates a nginx internal timer associated with the request. It means that the timer will be automatically cleared on the request destruction. And that the only one timer may be set at once. To be able to set more that one timer per request, see the the timers.js in `js/` folder of the module.

The `callback` must be a function (a closure) and `timeout` is specified in milliseconds.

On success returns `Nginx.OK`.

Example of a cascade timer setting:

	function handler (r)
	{
		r.sendHttpHeader('text/html')
		
		var count = 10
		
		function sayHello ()
		{
			r.print('Hello # ' + count + '!\n')
			r.flush()
			
			if (--count > 0)
				r.setTimer(sayHello, 250)
			else
				r.sendSpecial(Nginx.HTTP_LAST)
		}
		
		sayHello()
		
		return Nginx.OK
	}

produces (with 250ms delay for each line):

	Hello # 10!
	Hello # 9!
	Hello # 8!
	Hello # 7!
	Hello # 6!
	Hello # 5!
	Hello # 4!
	Hello # 3!
	Hello # 2!
	Hello # 1!

In this example nginx will wait till all the `sayHello()`'s will fire and only after that finalize the request.



### clearTimer()

Just cleares the timer if set. There is no arguments as far as the only one timer can be set per request (without a third-party library as timers.js).



### subrequest(uri, callback)

Subrequest in nginx are by no means an AJAX request. Subrequests are quite useless at the point as far as they share the same headers and variables set in same time being processed in parallel. In short use this if and only if you know what you are doing ;)

This methods creates a subrequest (a dependent request with shared almost everything) and directs it to the `uri`. After the subrequest is complete (always asynchronous) nginx will invoke the `callback`.

On the successful subrequest creation the method return a subrequest object (actually a `Nginx.Request` instance). It looks like a general request but it is not, be careful with it as far as we do not know all the cases in which it can crash ;)

The callback itself is very interesting part:

	function callback (sr, body, rc) { /* ... */ }

The callback takes three parameters: subrequest (`sr`) in which context it was invoked, the response body data (`body`) and the subrequest “result code” (`rc`). The last one is an internal nginx code and now is used for test purposes only.

This is a good idea to issue a subrequest throug the `proxy_passs` even to the nginx itself. This trick helps to deal with other modules in cost of establishing a loopback connection from nginx to nginx itself.

In nginx.conf:

	location /nginx.org
	{
		proxy_pass http://nginx.org/;
		proxy_buffers 3 512K;
		proxy_buffer_size 512K;
	}

the handler:

	function handler (r)
	{
		var uri = '/nginx.org'
		
		r.sendHttpHeader('text/plain; charset=utf-8')
		r.print('accessing ' + uri + '\n')
		r.flush()
		
		function callback (sr, body, rc)
		{
			r.print('got body with length ' + body.length + '\n')
			r.sendSpecial(Nginx.HTTP_LAST)
		}
		
		r.subrequest(uri, callback)
	}

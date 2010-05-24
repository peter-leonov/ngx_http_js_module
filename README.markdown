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
* [binary data API][#29] is needed: today we can response with a plain text only.

[#27]: http://github.com/kung-fu-tzu/ngx_http_js_module/issues/issue/27
[#28]: http://github.com/kung-fu-tzu/ngx_http_js_module/issues/issue/28
[#29]: http://github.com/kung-fu-tzu/ngx_http_js_module/issues/issue/29


sendHttpHeader(contentType)
---------------------------

Sends the header. In addition this method sets the response status to `200` if not it set, and sets the `headersOut["Content-Type"]` to the value of argument `contentType`.

	r.sendHttpHeader('text/html; charset=utf-8')


print(string)
-------------

Sends data of a UTF-8 encoded `string`. This method is able to send only text, not binary data. This means we can not send a string containing the binary zero symbol (i.e. `"\0"`). Also this method could not send an empty string, it just returns without doing any work.

	r.print('Hello, World!')


flush()
-------

This method is pretty simple, it just flushes the request output chain. In terms of nginx it sends a special buffer with a `flush` field set on.

	// sending the response body in two chunks
	r.print('Hello')
	r.flush()
	r.print(', World!')


sendString(string [, contentType])
----------------------------------

This is a combo-method. It does many thing at once: calculates the size in bytes of the `string`, sets the `Content-Length` header, sets an optional `Content-Type` header, then sends the response headers (like `sendHttpHeader()` does) and sends the `string` (like `print()` does). Overcomplicated? Yes, but all this must be done on the nginx side when the client doesn't support HTTP/1.1. The main thing in all this is that the JS can not calculate a real bytes count will be send on the wire. JS cal only tell us a count of UTF-16 characters in a string, while we need the count of bytes. In short, just do not use this method if you do not really need it ;)

	var body = ''
	// ...
	body += 'Hello'
	// ...
	body += ', World!'
	
	r.sendString(body, 'text/plain')


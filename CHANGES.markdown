0.2.14 (2010-06-04)
======

* new class `Dir` to rule the directories with `create()`, `remove()`, `removeTree()`, `walkTree()`, etc.;
* new methods in `Ngnx.File`: `getAccess()` and `setAccess()`;
* `HeadersOut#$contentLength` renamed to `HeadersOut#$contentLengthN` as in `HeadersIn` class.


0.2.13 (2010-05-27)
======

* support for debug build of the SpiderMonkey;
* `configure` now use nginx feature investigation tools to check the SpiderMonkey abailability and version;
* `File.exists()` now returns `null` if file does not exist and `false` if its not a file;
* `File.open()` now returns `null` if file does not exist.


0.2.12 (2010-05-19)
======

* support for nginx 0.8.37;
* `File.exists()` checks for an entry existency;
* good looking test reports;
* `HeadersOut#$contentTypeLen` and `HeadersOut#$contentTypeLowcase` properties are now marked as readonly as they are;
* `Request#sendString()` and `Request#sendHttpHeader()` convert an argument to string manually;
* the README! :)
* first tested with sporadically OOM situation (lots of fixes).
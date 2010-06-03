;(function(){

var Dir = Nginx.Dir,
	File = Nginx.File,
	prefix = Nginx.prefix

NginxTests.dir = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	
	Tests.test('tests for Nginx.Dir class', function (t)
	{
		t.test('create/delete', function (t)
		{
			var fname = prefix + 'nginx-dir-create'
			t.type(Dir, 'function', 'Nginx.Dir object')
			
			var access = 0700
			
			var rc = Dir.create(fname, access)
			t.ne(rc, File.ERROR, 'create')
			
			var rc = Dir.create(fname, access)
			t.eq(rc, File.ERROR, 'second create')
			
			t.eq(File.getAccess(fname), access, 'access')
			
			t.ne(File.setAccess(fname, access), File.ERROR, 'set access')
			t.eq(File.getAccess(fname), access, 'access')
			
			var rc = Dir.remove(fname)
			t.ne(rc, File.ERROR, 'delete')
			
			var rc = Dir.remove(fname)
			t.eq(rc, File.ERROR, 'second delete')
		})
		
		t.test('path', function (t)
		{
			var rc = Dir.createPath(prefix + 'a/b/c/ddd/', 0755)
			t.eq(rc, Nginx.OK, 'createPath()')
		})
	})
	Tests.oncomplete = function ()
	{
		r.puts('all done')
		r.flush()
		r.sendSpecial(Nginx.HTTP_LAST)
	}
	Tests.run(r)
	
	return Nginx.OK
}

})();
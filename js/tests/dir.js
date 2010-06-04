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
			
			var rc = Dir.removeTree(prefix + 'a/')
			t.eq(rc, Nginx.OK, 'removeTree()')
		})
		
		t.test('tree arguments', function (t)
		{
			var path = prefix + 'src/'
			
			function noop () {}
			
			t.exception(function (t)
			{
				Dir.walkTree()
			})
			
			t.exception(function (t)
			{
				Dir.walkTree(path, 2, 3, 4, 5, 6)
			})
			
			t.exception(function (t)
			{
				Dir.walkTree(path, 2, 3, 4, 5)
			})
			
			t.exception(function (t)
			{
				Dir.walkTree(path, noop, 3, 4, 5)
			})
			
			t.exception(function (t)
			{
				Dir.walkTree(path, noop, noop, 4, 5)
			})
			
			t.exception(function (t)
			{
				Dir.walkTree(path, noop, noop, noop, 5)
			})
			
			t.exception(function (t)
			{
				Dir.walkTree(path, null, null, null, 5)
			})
			
			t.eq(Dir.walkTree(path), Nginx.OK, 'path only')
			
			t.eq(Dir.walkTree(path, null, null, null, null), Nginx.OK, 'path with four nulls')
		})
		
		
		t.test('tree', function (t)
		{
			var stack = [],
				root = {name: 'root', type: 'dir', entrs: []},
				cur = root
			
			function fname (path)
			{
				var m = /([^\/]+)$/.exec(path)
				return m ? m[1] : '[error]'
			}
			
			function file (path)
			{
				cur.entrs.push({name: fname(path), type: 'file'})
			}
			
			function enterDir (path)
			{
				var dir = {name: fname(path), type: 'dir', entrs: []}
				cur.entrs.push(dir)
				
				stack.push(cur)
				cur = dir
			}
			
			function leaveDir (path)
			{
				cur.name2 = fname(path)
				cur = stack.pop()
			}
			
			function special (path)
			{
				cur.entrs.push({name: fname(path), type: 'special'})
			}
			
			Dir.walkTree(prefix + 'src/', file, enterDir, leaveDir, special)
			
			t.info(root.entrs)
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
;(function(){

var File = Nginx.File,
	prefix = Nginx.prefix

NginxTests.file = function (r)
{
	r.sendHttpHeader('text/plain; charset=utf-8')
	
	Tests.test('tests for Nginx.File object', function (t)
	{
		t.type(File, 'function', 'Nginx.File object')
		
		t.test('rename', function (t)
		{
			var rc = File.rename(prefix + 'nginx-file-rename.txt', prefix + 'nginx-file-rename.renamed.txt')
			t.ne(rc, File.ERROR, 'first rename')
			
			var rc = File.rename(prefix + 'nginx-file-rename.txt', prefix + 'nginx-file-rename.renamed.txt')
			t.eq(rc, File.ERROR, 'second rename')
			
			var rc = File.rename(prefix + 'nginx-file-rename.renamed.txt', prefix + 'nginx-file-rename.txt')
			t.ne(rc, File.ERROR, 'rename back')
		})
		
		t.test('create/delete/exists', function (t)
		{
			var fname = prefix + 'nginx-file-create.txt'
			
			t.eq(File.exists(prefix), false, 'directory exists')
			
			t.eq(File.exists(fname), null, 'not exists')
			
			var file = File.open(fname)
			t.ok(file, 'create')
			t.instance(file, File, 'file object')
			
			t.eq(File.exists(fname), true, 'exists')
			
			var rc = File.remove(fname)
			t.ne(rc, File.ERROR, 'delete')
			
			t.eq(File.exists(fname), null, 'not exists')
			
			var rc = File.remove(fname)
			t.eq(rc, File.ERROR, 'second delete')
			
			t.eq(File.exists(fname), null, 'not exists')
		})
		
		t.test('failing create', function (t)
		{
			var file = File.open(prefix)
			t.eq(file, null, 'File.open(dir)')
		})
		
		
		t.test('unicode read/write', function (t)
		{
			var fname = prefix + 'nginx-file-write.txt',
				str = 'Тра-ля-ля-ля и Тру-ля-ля!'
			
			var file = File.open(fname)
			t.ok(file, 'create')
			
			var rc = file.write(str)
			t.ok(rc, 'write')
			
			var file = File.open(fname)
			t.ok(file, 'open')
			
			var size = file.size
			t.gte(size, str.length, 'size')
			
			var res = file.read(size)
			t.eq(res, str, 'read')
			
			var rc = File.remove(fname)
			t.ne(rc, File.ERROR, 'delete')
			
			t.eq(File.readLeaks, 0, 'read leaks')
		})
		
		t.test('large read/write', function (t)
		{
			// 50 * 2000 = 100'000
			var str = new Array(2000).join('aaaaaaaaaabbbbbbbbbbcccccccccceeeeeeeeeeffffffffff'),
				fname = prefix + 'nginx-file-write.txt'
			
			var file = File.open(fname)
			t.ok(file, 'create')
			
			var rc = file.write(str)
			t.ok(rc, 'write')
			
			var size = file.size
			t.gte(size, str.length, 'size')
			
			var res = file.seek(0)
			t.eq(res, Nginx.OK, 'seek(0)')
			
			var res = file.read(size)
			t.eq(res.length, str.length, 'read length')
			t.eq(res, str, 'read data')
			
			var rc = File.remove(fname)
			t.ne(rc, File.ERROR, 'delete')
			
			t.eq(File.readLeaks, 0, 'read leaks')
		})
		
		t.test('close and openFiles', function (t)
		{
			var fname = prefix + 'nginx-file-close.txt',
				start = File.openFiles
			
			t.gte(start, 0, 'zero or more open files')
			
			var file = File.open(fname)
			t.eq(File.openFiles, start + 1, '+1 open')
			
			file.close()
			t.eq(File.openFiles, start, '+0 open')
			
			function closure ()
			{
				var file = File.open(fname)
				t.eq(File.openFiles, start + 1, '+1 open')
			}
			
			closure()
			GC()
			
			t.eq(File.openFiles, start, '+0 open')
			
			File.remove(fname)
		})
		
		
		t.test('access', function (t)
		{
			var fname = prefix + 'nginx-file-access.txt'
			
			var file = File.open(fname)
			t.ok(file, 'create')
			
			function checkAccess (access)
			{
				var rc = File.setAccess(fname, access)
				t.ne(rc, File.ERROR, 'setAccess()')
				
				var rc = File.getAccess(fname)
				t.ne(rc, File.ERROR, 'getAccess()')
				
				t.eq(rc, access, 'access')
			}
			
			checkAccess(0777)
			checkAccess(0765)
			checkAccess(0)
			checkAccess(0123)
			checkAccess(0666)
			
			var rc = File.remove(fname)
			t.ne(rc, File.ERROR, 'delete')
		})
		
		
		t.test('private protection', function (t)
		{
			var file = new File()
			
			t.exception(function (t)
			{
				file.size
			}, 'size')
			
			t.exception(function (t)
			{
				file.read()
			}, 'read()')
			
			t.exception(function (t)
			{
				File.prototype.read.call({}, 123)
			}, 'call({})')
			
			t.exception(function (t)
			{
				File.prototype.read.call(r, 123)
			}, 'call(request)')
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
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
		
		t.test('create/delete', function (t)
		{
			var file = File.open(prefix + 'nginx-file-create.txt')
			t.ok(file, 'create')
			t.instance(file, File, 'file object')
			
			var rc = File.remove(prefix + 'nginx-file-create.txt')
			t.ne(rc, File.ERROR, 'delete')
			
			var rc = File.remove(prefix + 'nginx-file-create.txt')
			t.eq(rc, File.ERROR, 'second delete')
		})
		
		t.test('write/read', function (t)
		{
			var str = 'Тра-ля-ля-ля и Тру-ля-ля!'
			
			var file = File.open(prefix + 'nginx-file-write.txt')
			t.ok(file, 'create')
			
			var rc = file.write(str)
			t.ok(rc, 'write')
			
			var file = File.open(prefix + 'nginx-file-write.txt')
			t.ok(file, 'open')
			
			var size = file.size
			t.gte(size, str.length, 'size')
			
			var res = file.read(size)
			t.eq(res, str, 'read')
			
			var rc = File.remove(prefix + 'nginx-file-write.txt')
			t.ne(rc, File.ERROR, 'delete')
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
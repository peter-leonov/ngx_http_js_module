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
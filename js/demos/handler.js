Hello =
{
	handler: function (r)
	{
		r.sendHttpHeader('text/html')
		
		if (r.headerOnly)
		{
			return Nginx.OK
		}
		
		r.print('hello!\n<br/>')
		
		if (Nginx.File.exists(r.filename))
		{
			r.print(' exists!\n')
		}
		
		r.sendSpecial(Nginx.HTTP_LAST)
		
		return Nginx.OK
	}
}

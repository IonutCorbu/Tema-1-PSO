build: so_stdio.dll

so_stdio.dll: so_stdio.obj 
	link /nologo /dll /out:so_stdio.dll /implib:so_stdio.lib so_stdio.obj 
	
so_stdio.obj: so_stdio.c so_stdio.h
	cl /c /MD /W1 /nologo so_stdio.c /Foso_stdio.obj

clean:
	del *.obj *.lib *.dll *exp
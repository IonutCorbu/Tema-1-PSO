build: libso_stdio.so
libso_stdio.so:  so_stdio.o
	gcc -shared so_stdio.o -o libso_stdio.so
	sudo cp libso_stdio.so /lib
so_stdio.o: so_stdio.c
	gcc -fPIC -c so_stdio.c -o so_stdio.o

clean: 
	rm so_stdio.o
	rm main.o
	rm libso_stdio.so



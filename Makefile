all: deamon WebServer deamon.o WebServer.o

deamon: src/deamon.c
		gcc -Wall -g src/deamon.c -o bin/deamon -ludev 

deamon.o: src/deamon.c
		gcc -Wall -c src/deamon.c -o obj/deamon.o -ludev 

WebServer: src/WebServer.c
		gcc -g -Wall src/WebServer.c -o bin/WebServer -I$PATH_TO_LIBMHD_INCLUDES  -L$PATH_TO_LIBMHD_LIBS -lmicrohttpd

WebServer.o: src/WebServer.c
		gcc -c -Wall src/WebServer.c -o obj/WebServer.0 -I$PATH_TO_LIBMHD_INCLUDES  -L$PATH_TO_LIBMHD_LIBS -lmicrohttpd

clean: 
		rm -rf obj/* bin/*

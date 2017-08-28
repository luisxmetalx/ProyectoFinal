all: deamon WebServer

deamon: deamon.c
		gcc -Wall -g src/deamon.c -o bin/deamon -ludev

WebServer: WebServer.c
		gcc -g -Wall src/WebServer.c -o bin/WebServer -I$PATH_TO_LIBMHD_INCLUDES  -L$PATH_TO_LIBMHD_LIBS -lmicrohttpd
clean: 
		rm -rf obj/* bin/*

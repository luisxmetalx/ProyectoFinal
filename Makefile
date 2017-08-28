all: deamon WebServer

deamon: src/deamon.c
		gcc -Wall -g src/deamon.c -o bin/deamon -ludev

WebServer: src/WebServer.c
		gcc -g -Wall src/WebServer.c -o bin/WebServer -I$PATH_TO_LIBMHD_INCLUDES  -L$PATH_TO_LIBMHD_LIBS -lmicrohttpd
clean: 
		rm -rf obj/* bin/*

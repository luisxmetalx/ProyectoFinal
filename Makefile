all: deamon WebServer

#demonio: demonio.c
#		gcc -Wall -g demonio.c -o demonio -ludev

deamon: deamon.c
		gcc -Wall -g deamon.c -o deamon -ludev

WebServer: WebServer.c
		gcc -g -Wall WebServer.c -o WebServer -I$PATH_TO_LIBMHD_INCLUDES  -L$PATH_TO_LIBMHD_LIBS -lmicrohttpd
clean: 
		rm -rf deamon WebServer
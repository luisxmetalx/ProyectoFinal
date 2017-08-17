all: demonio

demonio: demonio.c
		gcc -Wall -g demonio.c -o demonio -ludev
clean: 
		rm -rf demonio
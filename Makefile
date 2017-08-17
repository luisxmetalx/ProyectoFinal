all: demonio demonio2

demonio: demonio.c
		gcc -Wall -g demonio.c -o demonio -ludev

demonio2: demonio2.c
		gcc -Wall -g demonio2.c -o demonio2 -ludev
clean: 
		rm -rf demonio demonio2
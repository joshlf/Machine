all:
	gcc -std=c99 main.c machine.c -o machine

debug:
	gcc -DDEBUG -std=c99 main.c machine.c -o machine

clean:
	rm machine
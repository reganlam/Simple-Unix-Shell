CC=gcc
CFLAGS=

all: my_shell
program: my_shell.o
mycode.o: my_shell.c

clean:
	rm -f my_shell my_shell.o
run: my_shell
	./my_shell

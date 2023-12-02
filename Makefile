CC = gcc
CFLAGS = -c -g


vite : vite.c
	$(CC) vite.c -o vite

clean :
	rm -f vite
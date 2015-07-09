CC = gcc
CFLAGS = -I . -Wall

fileexplorer: fileexplorer.o
	$(CC) -o fileexplorer main.o $(CFLAGS)

fileexplorer.o:
	$(CC) -o fileexplorer main.c $(CFLAGS)

clean:
	rm *.o

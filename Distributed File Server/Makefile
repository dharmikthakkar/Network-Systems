CC=gcc
CFLAGS = -g 
LIBS = -lcrypto

all: dfs dfc

dfc: dfc.o 
	$(CC) $(CFLAGS) -o dfc dfc.o $(LIBS)

dfs: dfs.o 
	$(CC) $(CFLAGS) -o dfs dfs.o

dfc.o: dfc.c

dfs.o: dfs.c

clean:
	rm -rf dfs dfc dfc.o dfs.o DFS1 DFS2 DFS3 DFS4 DFC .foo*

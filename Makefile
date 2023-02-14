all: expcounter
LIBS=-lsocket -lnsl
TYPE=unspecified
CFLAGS=-g 
CC=gcc

expcounter:  proctype.h expcounter.o sock_init.o getexpinfo.o
	$(CC) $(CFLAGS) -o expcounter expcounter.o sock_init.o getexpinfo.o $(LIBS)

expcounterd:  expcounterd.o sock_init.o getexpinfo.o
	$(CC) $(CFLAGS) -o expcounterd expcounterd.o sock_init.o getexpinfo.o $(LIBS)

proctype.h:
	echo '#define PROCTYPE "'$(TYPE)'"' >> proctype.h

clean:
	rm -rf *.o expcounter

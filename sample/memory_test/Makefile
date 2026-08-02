PROGRAM = qs_memory_test
CC      = gcc
DEFINE  = -D_LINUX
CFLAG   = -pthread -Wall -O
INCDIR  = -I ../../core/header
LIBA = ../../core/libqs_core.a
MYSOCKETOBJGROUP = main.o $(LIBA)

build: $(PROGRAM)

$(LIBA):
	cd ../../core/src && make

$(PROGRAM): $(MYSOCKETOBJGROUP)
	$(CC) $(CFLAG) $(INCDIR) -o $(PROGRAM) $(MYSOCKETOBJGROUP)

main.o: main.c
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c main.c -o main.o

clean:
	rm -f *.o $(PROGRAM)

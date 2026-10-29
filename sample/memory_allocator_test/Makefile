PROGRAM = qs_memory_allocator_test
CC      = gcc
DEFINE  = -D_LINUX
CFLAG   = -pthread -Wall -O
INCDIR  = -I ../../core/header
LIBA    = ../../core/libqs_core.a
OBJS    = main.o $(LIBA)

build: $(PROGRAM)

$(LIBA):
	cd ../../core/src && make

$(PROGRAM): $(OBJS)
	$(CC) $(CFLAG) $(INCDIR) -o $(PROGRAM) $(OBJS)

main.o: main.c
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c main.c -o main.o

.PHONY: clean
clean:
	cd ../../core/src && make clean
	rm -f $(PROGRAM)
	rm -f ./*.o

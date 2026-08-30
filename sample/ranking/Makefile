PROGRAM = qs_ranking
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
	$(CC) $(CFLAG) $(LIBDIR) $(INCDIR) -o $(PROGRAM) $(MYSOCKETOBJGROUP)

main.o: main.c
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c main.c -o main.o

.PHONY: clean
clean: 
	cd ../../core/src && make clean
	rm -f $(PROGRAM) 
	rm -f ./*.o

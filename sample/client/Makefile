PROGRAM = gdt_client
CC      = gcc
DEFINE  = -D_LINUX
CFLAG   = -pthread -Wall -O
#CFLAG   = -Wall -O2
INCDIR  = -I ../../core/header
LIBA = ../../core/libgdt_core.a
MYSOCKETOBJGROUP = main.o $(LIBA)

build: $(PROGRAM)

$(PROGRAM): $(MYSOCKETOBJGROUP)
	$(CC) $(CFLAG) $(LIBDIR) $(INCDIR) -o $(PROGRAM) $(MYSOCKETOBJGROUP)

main.o: main.c
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c main.c -o main.o

.PHONY: clean
clean: 
	rm -f $(PROGRAM) 
	rm -f ./*.o

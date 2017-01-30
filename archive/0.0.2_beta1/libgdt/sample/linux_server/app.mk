PROGRAM = gdt_echo_server
CC      = gcc
CFLAG   = -g -pthread -Wall -O
INCDIR  = -I ./header -I ../../core/header 
LIBDIR  = -L ../../core/ 
CORELIB = ../../core/libgdt_core.a
MYLIBRARYDIR = ../../
SRCDIR  = ./src/
MYSOCKETOBJGROUP = main.o $(CORELIB)

build: $(PROGRAM)

$(PROGRAM): $(MYSOCKETOBJGROUP)
	$(CC) $(CFLAG) $(LIBDIR) $(INCDIR) -o $(PROGRAM) $(MYSOCKETOBJGROUP)

main.o: main.c
	$(CC) $(CFLAG) $(LIBDIR) $(INCDIR) -c main.c -o main.o

.PHONY: clean
clean: 
	\rm -f $(PROGRAM) 
	\rm -f ./*.o

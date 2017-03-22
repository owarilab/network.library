PROGRAM = gdt_server
CC      = gcc
DEFINE  = -D"_LINUX"
CFLAG   = -g -pthread -Wall -O
INCDIR  = -I ./header -I ../header
LIBDIR  = 
LIBA = ./libgdt_core.a 
MYLIBRARYDIR = ../
SRCDIR  = ./src/
MYSOCKETOBJGROUP = main.o $(LIBA)

build: $(PROGRAM)

$(PROGRAM): $(MYSOCKETOBJGROUP)
	$(CC) $(CFLAG) $(LIBDIR) $(INCDIR) -o $(PROGRAM) $(MYSOCKETOBJGROUP)

main.o: main.c
	$(CC) $(DEFINE) $(CFLAG) $(LIBDIR) $(INCDIR) -c main.c -o main.o

.PHONY: clean
clean: 
	\rm -f $(PROGRAM) 
	\rm -f ./*.o

PROGRAM = gdt_http_server
CC      = gcc
DEFINE  = -D"_BSD_UNIX"
CFLAG   = -g -pthread -Wall -O
#CFLAG   = -Wall -O2
INCDIR  = -I ../../header
LIBDIR  = 
LIBA = ../libgdt_core.a
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

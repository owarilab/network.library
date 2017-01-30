PROGRAM = gdt_client
CC      = gcc
CFLAG   = -g -lpthread -pthread -Wall -O
INCDIR  = -I ../../core/header 
LIBDIR  = -L ../../core  
CORELIB = ../../core/libgdt_core.a
MYLIBRARYDIR = ../../
MYSOCKETOBJGROUP = main.o $(CORELIB)

build: $(PROGRAM)

clean: 
	\rm -f $(PROGRAM) 
	\rm -f ./*.o

$(PROGRAM): $(MYSOCKETOBJGROUP)
	$(CC) $(CFLAG) $(LIBDIR) $(INCDIR) -o $(PROGRAM) $(MYSOCKETOBJGROUP)

main.o: main.c
	$(CC) $(CFLAG) $(LIBDIR) $(INCDIR) -c main.c -o main.o 


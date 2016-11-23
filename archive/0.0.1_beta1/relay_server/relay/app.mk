PROGRAM = relay 
CC      = gcc
CFLAG   = -g -lpthread -pthread -Wall -O
INCDIR  = -I ./header -I ../../lib/core/header
LIBDIR  = -L ../../lib/core/header
CORELIB = ../../lib/core
MYLIBRARYDIR = ../../lib/
MYSOCKETOBJGROUP = main.o $(CORELIB)/core.a

build: $(PROGRAM)

clean: 
	\rm $(PROGRAM) 
	\rm ./*.o

$(PROGRAM): $(MYSOCKETOBJGROUP)
	$(CC) $(CFLAG) $(LIBDIR) $(INCDIR) -o $(PROGRAM) $(MYSOCKETOBJGROUP)

main.o: main.c
	$(CC) $(CFLAG) $(LIBDIR) $(INCDIR) -c main.c -o main.o 


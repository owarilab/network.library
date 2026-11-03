PROGRAM = issue_tracker_cli_test
CC      = gcc
CFLAG   = -pthread -Wall -O
INCDIR  = -I ../../core/header
LIBA    = ../../core/libqs_core.a
OBJS    = main.o

build: $(PROGRAM)

$(LIBA):
	cd ../../core/src && make

$(PROGRAM): $(OBJS) $(LIBA)
	$(CC) $(CFLAG) -o $(PROGRAM) $(OBJS) $(LIBA)

main.o: main.c
	$(CC) $(CFLAG) $(INCDIR) -c main.c -o main.o

.PHONY: clean
clean:
	rm -f $(PROGRAM)
	rm -f ./*.o

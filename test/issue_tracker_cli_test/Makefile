PROGRAM = issue_tracker_cli_test
CC      = gcc
CFLAG   = -Wall -O
OBJS    = main.o

build: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) $(CFLAG) -o $(PROGRAM) $(OBJS)

main.o: main.c
	$(CC) $(CFLAG) -c main.c -o main.o

.PHONY: clean
clean:
	rm -f $(PROGRAM)
	rm -f ./*.o

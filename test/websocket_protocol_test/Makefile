PROGRAM = qs_websocket_protocol_test
CC = gcc
CFLAG = -Wall -O
INCDIR = -I ../../core/header
LIBA = ../../core/libqs_core.a

all: $(PROGRAM)

$(LIBA):
	$(MAKE) -C ../../core/src

$(PROGRAM): main.o $(LIBA)
	$(CC) $(CFLAG) $(INCDIR) -o $@ main.o $(LIBA)

main.o: main.c
	$(CC) $(CFLAG) $(INCDIR) -c $< -o $@

.PHONY: clean test
test: $(PROGRAM)
	./$(PROGRAM)

clean:
	rm -f $(PROGRAM) main.o
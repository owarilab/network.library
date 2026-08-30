PROGRAM = qs_websocket_client
CC = gcc
CFLAG = -Wall -O
INCDIR = -I ../../core/header
LIBA = ../../core/libqs_core.a

all: $(PROGRAM)

$(LIBA):
	cd ../../core/src && make

$(PROGRAM): main.o $(LIBA)
	$(CC) $(CFLAG) $(INCDIR) -o $@ main.o $(LIBA) -pthread

main.o: main.c
	$(CC) $(CFLAG) $(INCDIR) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(PROGRAM) main.o
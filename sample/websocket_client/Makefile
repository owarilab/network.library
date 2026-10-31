PROGRAM = qs_websocket_client
CC = gcc
CFLAG = -Wall -O
INCDIR = -I ../../core/header -I ../../ssl/header
LIBA = ../../core/libqs_core.a ../../ssl/libqs_ssl_module.a

all: $(LIBA) $(PROGRAM)

../../core/libqs_core.a:
	cd ../../core/src && make

../../ssl/libqs_ssl_module.a:
	cd ../../ssl/src && make build

$(PROGRAM): main.o $(LIBA)
	$(CC) $(CFLAG) $(INCDIR) -o $@ main.o $(LIBA) -lssl -lcrypto -pthread

main.o: main.c
	$(CC) $(CFLAG) $(INCDIR) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(PROGRAM) main.o
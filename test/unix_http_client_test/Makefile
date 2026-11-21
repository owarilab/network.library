PROGRAM = qs_unix_http_client_test
CC = gcc
CFLAG = -Wall -O -pthread
INCDIR = -I ../../core/header -I ../../ssl/header
LIBA = ../../core/libqs_core.a ../../ssl/libqs_ssl_module.a -lssl -lcrypto

all: $(PROGRAM)

$(LIBA):
	$(MAKE) -C ../../core/src
	$(MAKE) -C ../../ssl/src

$(PROGRAM): main.o $(LIBA)
	$(CC) $(CFLAG) $(INCDIR) -o $@ main.o $(LIBA)

main.o: main.c
	$(CC) $(CFLAG) $(INCDIR) -c $< -o $@

.PHONY: clean test
test: $(PROGRAM)
	./$(PROGRAM)

clean:
	rm -f $(PROGRAM) main.o

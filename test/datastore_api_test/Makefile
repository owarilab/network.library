CC=gcc
CFLAGS=-pthread -Wall -O0 -I ../../core/header
LDFLAGS=

all: qs_datastore_api_test

qs_datastore_api_test: main.o ../../core/libqs_core.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ main.o ../../core/libqs_core.a

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

../../core/libqs_core.a:
	$(MAKE) -C ../../core/src

clean:
	rm -f qs_datastore_api_test main.o

.PHONY: all clean

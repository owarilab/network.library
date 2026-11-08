PROGRAM = qs_datastore_test
CC      = gcc
DEFINE  = -D_LINUX
CFLAG   = -pthread -Wall -O
INCDIR  = -I ../../core/header
LIBA = ../../core/libqs_core.a
MYSOCKETOBJGROUP = main.o $(LIBA)

build: $(PROGRAM)

$(LIBA):
	cd ../../core/src && make

$(PROGRAM): $(MYSOCKETOBJGROUP)
	$(CC) $(CFLAG) $(LIBDIR) $(INCDIR) -Wl,--wrap=qs_create_storage_cache -Wl,--wrap=qs_remove_hash -Wl,--wrap=qs_array_push_string -Wl,--wrap=qs_json_encode_array -o $(PROGRAM) $(MYSOCKETOBJGROUP)

main.o: main.c
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c main.c -o main.o

.PHONY: clean
clean:
	cd ../../core/src && make clean
	rm -f $(PROGRAM)
	rm -f ./*.o

PROGRAM = qs_simple_client
CC      = gcc
DEFINE  = -D_LINUX
CFLAG   = -pthread -Wall -O
#CFLAG   = -Wall -O2
INCDIR  = -I ../../core/header -I ../../ssl/header -I/usr/include/openssl
LIBA = ../../core/libqs_core.a ../../ssl/libqs_ssl_module.a -lssl -lcrypto
MYSOCKETOBJGROUP = main.o $(LIBA)

build: $(PROGRAM)

$(LIBA):
	cd ../../core/src && make 

$(PROGRAM): $(MYSOCKETOBJGROUP)
	$(CC) $(CFLAG) $(LIBDIR) $(INCDIR) -o $(PROGRAM) $(MYSOCKETOBJGROUP)

main.o: main.c
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c main.c -o main.o

.PHONY: clean
clean: 
	cd ../../core/src && make clean
	rm -f $(PROGRAM) 
	rm -f ./*.o

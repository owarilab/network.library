PROGRAM = qs_http_client
UNAME_S := $(shell uname -s)
CC      = $(if $(filter Darwin,$(UNAME_S)),clang,gcc)
DEFINE  = $(if $(filter Darwin,$(UNAME_S)),,-D_LINUX)
CFLAG   = -pthread -Wall -O
#CFLAG   = -Wall -O2
OPENSSL_PREFIX ?= $(if $(filter Darwin,$(UNAME_S)),$(shell brew --prefix openssl@3 2>/dev/null),)
INCDIR  = -I ../../core/header -I ../../ssl/header -I $(if $(OPENSSL_PREFIX),$(OPENSSL_PREFIX)/include,/usr/include/openssl)
LIBDIR  = $(if $(OPENSSL_PREFIX),-L$(OPENSSL_PREFIX)/lib,)
LIBA = ../../core/libqs_core.a ../../ssl/libqs_ssl_module.a -lssl -lcrypto
MYSOCKETOBJGROUP = main.o $(LIBA)

build: $(PROGRAM)

$(LIBA):
	cd ../../core/src && make 
	cd ../../ssl/src && make

$(PROGRAM): $(MYSOCKETOBJGROUP)
	$(CC) $(CFLAG) $(LIBDIR) $(INCDIR) -o $(PROGRAM) $(MYSOCKETOBJGROUP)

main.o: main.c
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c main.c -o main.o

.PHONY: clean
clean: 
	cd ../../core/src && make clean
	cd ../../ssl/src && make clean
	rm -f $(PROGRAM) 
	rm -f ./*.o

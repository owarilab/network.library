PROGRAM = libqs_ssl_module.a
PROGRAMOUT=../
UNAME_S := $(shell uname -s)
CC      = $(if $(filter Darwin,$(UNAME_S)),clang,gcc)
AR=ar
DEFINE  = $(if $(filter Darwin,$(UNAME_S)),,-D_LINUX)
CFLAG   = -pthread -Wall -O
#CFLAG   = -Wall -O2
OPENSSL_PREFIX ?= $(if $(filter Darwin,$(UNAME_S)),$(shell brew --prefix openssl@3 2>/dev/null),)
INCDIR  = -I ../header -I ../../core/header -I $(if $(OPENSSL_PREFIX),$(OPENSSL_PREFIX)/include,/usr/include/openssl)
SSL_LIBS = -L$(OPENSSL_PREFIX)/lib -lssl -lcrypto
LIBA = 
OBJGROUP = qs_openssl_module.o $(LIBA)

build: $(PROGRAM)

$(LIBA):
	cd ../../core/src && make 

$(PROGRAM): $(OBJGROUP)
	$(AR) r  $(PROGRAMOUT)$(PROGRAM) $(OBJGROUP)

qs_openssl_module.o: qs_openssl_module.c
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c qs_openssl_module.c -o qs_openssl_module.o

.PHONY: clean
clean: 
	cd ../../core/src && make clean
	rm -f $(PROGRAMOUT)$(PROGRAM)
	rm -f ./*.o

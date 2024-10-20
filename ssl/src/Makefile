PROGRAM = libqs_ssl_module.a
PROGRAMOUT=../
CC      = gcc
AR=ar
DEFINE  = -D_LINUX
CFLAG   = -pthread -Wall -O
#CFLAG   = -Wall -O2
INCDIR  = -I ../header -I ../../core/header -I/usr/include/openssl
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

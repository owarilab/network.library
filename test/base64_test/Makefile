PROGRAM = qs_base64_test
PROGRAM2 = qs_verify_decode
CC      = gcc
DEFINE  = -D_LINUX
CFLAG   = -pthread -Wall -O
INCDIR  = -I ../../core/header
LIBA    = ../../core/libqs_core.a
OBJS    = main.o $(LIBA)
OBJS2   = verify_decode.o $(LIBA)

build: $(PROGRAM) $(PROGRAM2)

$(LIBA):
	cd ../../core/src && make

$(PROGRAM): $(OBJS)
	$(CC) $(CFLAG) $(INCDIR) -o $(PROGRAM) $(OBJS)

$(PROGRAM2): $(OBJS2)
	$(CC) $(CFLAG) $(INCDIR) -o $(PROGRAM2) $(OBJS2)

main.o: main.c
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c main.c -o main.o

verify_decode.o: verify_decode.c
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c verify_decode.c -o verify_decode.o

.PHONY: clean
clean:
	cd ../../core/src && make clean
	rm -f $(PROGRAM) $(PROGRAM2)
	rm -f ./*.o

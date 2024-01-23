PROGRAM = libqs.dll
CC      = x86_64-w64-mingw32-gcc
DEFINE  = -D_WINDOWS
INCDIR  = -I ../../core/header
CFLAG   = $(INCDIR) $(DEFINE) -Wall -O 
LIBA = ../../core/libqs_core_win.a
MYSOCKETOBJGROUP = libqs.o $(LIBA)

all: $(PROGRAM)

$(LIBA):
	cd ../../core/src && make -f Makefile_Windows

$(PROGRAM): $(MYSOCKETOBJGROUP)
	$(CC) $(CFLAG) -shared $(LIBDIR) -o $(PROGRAM) $(MYSOCKETOBJGROUP) -lws2_32

libqs.o: libqs.c
	$(CC) $(CFLAG) -c libqs.c -o libqs.o

.PHONY: clean
clean: 
	cd ../../core/src && make -f Makefile_Windows clean
	rm -f $(PROGRAM) 
	rm -f ./*.o

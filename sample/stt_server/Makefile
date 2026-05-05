PROGRAM = qs_stt_server
CC      = gcc
DEFINE  = -D_LINUX
CFLAG   = -pthread -Wall -O
#CFLAG   = -Wall -O2
INCDIR  = -I ../../core/header
LIBA = ../../core/libqs_core.a
WHISPER_INCDIR = -I ../../stt/headers
WHISPER_LIBDIR = -L ../../stt/libs
WHISPER_LIBS = -lwhisper -lggml -lggml-base -lggml-cpu
INCDIR += $(WHISPER_INCDIR)
CFLAG += $(WHISPER_LIBDIR)
MYSOCKETOBJGROUP = main.o $(LIBA)

build: $(PROGRAM)

$(LIBA):
	cd ../../core/src && make 

$(PROGRAM): $(MYSOCKETOBJGROUP)
	$(CC) $(CFLAG) $(LIBDIR) $(INCDIR) -o $(PROGRAM) $(MYSOCKETOBJGROUP) $(WHISPER_LIBS) -lm

main.o: main.c
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c main.c -o main.o

.PHONY: clean
clean: 
	cd ../../core/src && make clean
	rm -f $(PROGRAM) 
	rm -f ./*.o

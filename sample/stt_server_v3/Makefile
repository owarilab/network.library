PROGRAM = qs_stt_vad_server
CC      = gcc
DEFINE  = -D_LINUX
CFLAG   = -pthread -Wall -O2
INCDIR  = -I ../../core/header
LIBA    = ../../core/libqs_core.a
WHISPER_INCDIR = -I ../../stt/headers
WHISPER_LIBDIR = -L ../../stt/libs
WHISPER_RPATH  = -Wl,-rpath,'$$ORIGIN/../../stt/libs'
WHISPER_LIBS   = -lwhisper -lggml -lggml-base -lggml-cpu $(WHISPER_RPATH)
INCDIR += $(WHISPER_INCDIR)
MYSOCKETOBJGROUP = main.o $(LIBA)

build: $(PROGRAM)

$(LIBA):
	cd ../../core/src && make

$(PROGRAM): $(MYSOCKETOBJGROUP)
	$(CC) $(CFLAG) $(WHISPER_LIBDIR) -o $(PROGRAM) $(MYSOCKETOBJGROUP) $(WHISPER_LIBS) -lm

main.o: main.c
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c main.c -o main.o

.PHONY: clean
clean:
	rm -f $(PROGRAM)
	rm -f ./*.o

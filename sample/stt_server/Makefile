PROGRAM = qs_stt_server
WAV_TEST_PROGRAM = qs_stt_wav_test
CC      = gcc
DEFINE  = -D_LINUX
CFLAG   = -pthread -Wall -O
#CFLAG   = -Wall -O2
INCDIR  = -I ../../core/header
LIBA = ../../core/libqs_core.a
WHISPER_INCDIR = -I ../../stt/headers
WHISPER_LIBDIR = -L ../../stt/libs
WHISPER_RPATH = -Wl,-rpath,'$$ORIGIN/../../stt/libs'
WHISPER_LIBS = -lwhisper -lggml -lggml-base -lggml-cpu $(WHISPER_RPATH)
INCDIR += $(WHISPER_INCDIR)
CFLAG += $(WHISPER_LIBDIR)
MYSOCKETOBJGROUP = main.o $(LIBA)

build: $(PROGRAM)

test_wav: $(WAV_TEST_PROGRAM)

test_wav_clean:
	rm -f $(WAV_TEST_PROGRAM)

$(LIBA):
	cd ../../core/src && make 

$(PROGRAM): $(MYSOCKETOBJGROUP)
	$(CC) $(CFLAG) $(LIBDIR) $(INCDIR) -o $(PROGRAM) $(MYSOCKETOBJGROUP) $(WHISPER_LIBS) -lm

$(WAV_TEST_PROGRAM): test_wav.c
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -o $(WAV_TEST_PROGRAM) test_wav.c $(WHISPER_LIBS) -lm

main.o: main.c
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c main.c -o main.o

.PHONY: clean
clean: 
	cd ../../core/src && make clean
	rm -f $(PROGRAM) $(WAV_TEST_PROGRAM)
	rm -f ./*.o

.PHONY: test_wav test_wav_clean

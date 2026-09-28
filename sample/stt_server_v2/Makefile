PROGRAM = qs_stt_server_v2
VAD_PROBE = qs_stt_vad_probe
CC = gcc
DEFINE = -D_LINUX
CFLAGS = -pthread -Wall -Wextra -O2
CORE_LIB = ../../core/libqs_core.a
WHISPER_INCLUDES = -I../../stt/headers
WHISPER_LIBDIR = -L../../stt/libs
WHISPER_RPATH = -Wl,-rpath,'$$ORIGIN/../../stt/libs'
WHISPER_LIBS = -lwhisper -lggml -lggml-base -lggml-cpu $(WHISPER_RPATH)
INCLUDES = -I../../core/header $(WHISPER_INCLUDES)

.PHONY: all clean vad_probe run

all: transcripts $(PROGRAM)

$(CORE_LIB):
	$(MAKE) -C ../../core/src

$(PROGRAM): main.o $(CORE_LIB)
	$(CC) $(CFLAGS) $(WHISPER_LIBDIR) -o $@ main.o $(CORE_LIB) $(WHISPER_LIBS) -lm -pthread

main.o: main.c
	$(CC) $(DEFINE) $(CFLAGS) $(INCLUDES) -c $< -o $@

vad_probe: $(VAD_PROBE)
	./$(VAD_PROBE) ../../stt/models/ggml-silero-v5.1.2.bin ../stt_server/backup/recv_1bMTow0rn6qNd5LxfjbnsucbvCgf7xB8_2.wav

run: $(PROGRAM)
	./$(PROGRAM)

$(VAD_PROBE): vad_probe.c
	$(CC) $(DEFINE) $(CFLAGS) $(WHISPER_LIBDIR) $(WHISPER_INCLUDES) -o $@ $< $(WHISPER_LIBS) -lm

clean:
	rm -f $(PROGRAM) $(VAD_PROBE) main.o

transcripts:
	mkdir -p $@
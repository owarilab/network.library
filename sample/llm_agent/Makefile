PROGRAM = qs_llm_agent_server
CC      = gcc
DEFINE  = -D_LINUX
CFLAG   = -pthread -Wall -O
LLAMA_ENABLE ?= 0
LLAMA_CUDA ?= 1
LLAMA_CPP_DIR ?= ../../llm/third_party/llama.cpp
LLAMA_BUILD_DIR ?= $(LLAMA_CPP_DIR)/build
LLAMA_LIB_DIR ?= $(LLAMA_BUILD_DIR)/bin

ifeq ($(LLAMA_ENABLE),1)
DEFINE += -DQS_LLM_WITH_LLAMA_CPP=1
LLAMA_LIBS = -L $(LLAMA_LIB_DIR) -Wl,-rpath,$(abspath $(LLAMA_LIB_DIR)) -lllama
LLAMA_RUNTIME_TARGET = $(LLAMA_LIB_DIR)/libllama.so
LLAMA_CMAKE_ACCEL =
ifeq ($(LLAMA_CUDA),1)
LLAMA_CMAKE_ACCEL += -DGGML_CUDA=ON
endif
endif

INCDIR  = -I ../../core/header -I ../../llm/header -I ../../ssl/header -I /usr/include/openssl -I $(LLAMA_CPP_DIR)/include
LIBA    = ../../core/libqs_core.a ../../llm/libqs_llm_module.a ../../ssl/libqs_ssl_module.a -lssl -lcrypto

OBJGROUP = main.o agent_core.o \
           tools/tool_common.o tools/tool_file_list.o tools/tool_file_read.o tools/tool_file_write.o tools/tool_file_edit.o tools/tool_file_search.o tools/tool_grep_search.o \
           tools/tool_http_request.o tools/tool_url_whitelist_get.o tools/tool_url_whitelist_info_get.o \
           handlers/handler_common.o \
           handlers/handler_init.o handlers/handler_think.o \
           handlers/handler_execute.o handlers/handler_loop.o \
           handlers/handler_run.o handlers/handler_run_stream.o \
           $(LIBA)

build: $(LLAMA_RUNTIME_TARGET) $(PROGRAM)

$(LLAMA_LIB_DIR)/libllama.so:
	cd $(LLAMA_CPP_DIR) && cmake -S . -B build -DBUILD_SHARED_LIBS=ON -DLLAMA_BUILD_TESTS=OFF -DLLAMA_BUILD_EXAMPLES=OFF $(LLAMA_CMAKE_ACCEL)
	cd $(LLAMA_CPP_DIR) && cmake --build build -j4

../../core/libqs_core.a:
	cd ../../core/src && make

../../llm/libqs_llm_module.a:
	cd ../../llm/src && make build LLAMA_ENABLE=$(LLAMA_ENABLE)

../../ssl/libqs_ssl_module.a:
	cd ../../ssl/src && make build

$(PROGRAM): $(OBJGROUP)
	$(CC) $(CFLAG) $(INCDIR) -o $(PROGRAM) $(OBJGROUP) $(LLAMA_LIBS)

main.o: main.c
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c main.c -o main.o

agent_core.o: agent_core.c agent_core.h
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c agent_core.c -o agent_core.o

tools/tool_common.o: tools/tool_common.c tools/tool_common.h
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c tools/tool_common.c -o tools/tool_common.o

tools/tool_file_list.o: tools/tool_file_list.c tools/tool_file_list.h
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c tools/tool_file_list.c -o tools/tool_file_list.o

tools/tool_file_read.o: tools/tool_file_read.c tools/tool_file_read.h
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c tools/tool_file_read.c -o tools/tool_file_read.o

tools/tool_file_write.o: tools/tool_file_write.c tools/tool_file_write.h
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c tools/tool_file_write.c -o tools/tool_file_write.o

tools/tool_file_edit.o: tools/tool_file_edit.c tools/tool_file_edit.h
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c tools/tool_file_edit.c -o tools/tool_file_edit.o

tools/tool_file_search.o: tools/tool_file_search.c tools/tool_file_search.h
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c tools/tool_file_search.c -o tools/tool_file_search.o

tools/tool_grep_search.o: tools/tool_grep_search.c tools/tool_grep_search.h
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c tools/tool_grep_search.c -o tools/tool_grep_search.o

tools/tool_http_request.o: tools/tool_http_request.c tools/tool_http_request.h
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c tools/tool_http_request.c -o tools/tool_http_request.o

tools/tool_url_whitelist_get.o: tools/tool_url_whitelist_get.c tools/tool_url_whitelist_get.h
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c tools/tool_url_whitelist_get.c -o tools/tool_url_whitelist_get.o

tools/tool_url_whitelist_info_get.o: tools/tool_url_whitelist_info_get.c tools/tool_url_whitelist_info_get.h
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c tools/tool_url_whitelist_info_get.c -o tools/tool_url_whitelist_info_get.o

handlers/handler_common.o: handlers/handler_common.c handlers/handler_common.h
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c handlers/handler_common.c -o handlers/handler_common.o

handlers/handler_init.o: handlers/handler_init.c
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c handlers/handler_init.c -o handlers/handler_init.o

handlers/handler_think.o: handlers/handler_think.c
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c handlers/handler_think.c -o handlers/handler_think.o

handlers/handler_execute.o: handlers/handler_execute.c
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c handlers/handler_execute.c -o handlers/handler_execute.o

handlers/handler_loop.o: handlers/handler_loop.c
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c handlers/handler_loop.c -o handlers/handler_loop.o

handlers/handler_run.o: handlers/handler_run.c
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c handlers/handler_run.c -o handlers/handler_run.o

handlers/handler_run_stream.o: handlers/handler_run_stream.c
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c handlers/handler_run_stream.c -o handlers/handler_run_stream.o

.PHONY: clean
clean:
	rm -f $(PROGRAM)
	rm -f *.o
	rm -f tools/*.o
	rm -f handlers/*.o

PROGRAM = qs_llm_http_stream_server
CC      = gcc
DEFINE  = -D_LINUX
CFLAG   = -pthread -Wall -O
LLAMA_ENABLE ?= 0
QS_EMBEDDING_MODULE_ENABLED ?= 0
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

ifeq ($(QS_EMBEDDING_MODULE_ENABLED),1)
DEFINE += -DQS_EMBEDDING_MODULE_ENABLED=1 -DQS_EMBEDDING_WITH_LLAMA_CPP=1
EMBEDDING_LIBS = -lsqlite3 -lm
else
EMBEDDING_LIBS =
endif

INCDIR  = -I ../../core/header -I ../../llm/header -I $(LLAMA_CPP_DIR)/include
LIBA = ../../core/libqs_core.a ../../llm/libqs_llm_module.a
OBJGROUP = main.o $(LIBA)

build: $(LLAMA_RUNTIME_TARGET) $(PROGRAM)

$(LLAMA_LIB_DIR)/libllama.so:
	cd $(LLAMA_CPP_DIR) && cmake -S . -B build -DBUILD_SHARED_LIBS=ON -DLLAMA_BUILD_TESTS=OFF -DLLAMA_BUILD_EXAMPLES=OFF $(LLAMA_CMAKE_ACCEL)
	cd $(LLAMA_CPP_DIR) && cmake --build build -j4

../../core/libqs_core.a:
	cd ../../core/src && make

../../llm/libqs_llm_module.a:
	cd ../../llm/src && make build LLAMA_ENABLE=$(LLAMA_ENABLE)

../../llm/libqs_embedding_module.a:
	cd ../../llm/src && make build LLAMA_ENABLE=$(LLAMA_ENABLE) QS_EMBEDDING_MODULE_ENABLED=$(QS_EMBEDDING_MODULE_ENABLED)

$(PROGRAM): $(OBJGROUP)
	$(CC) $(CFLAG) $(INCDIR) -o $(PROGRAM) $(OBJGROUP) $(LLAMA_LIBS) $(EMBEDDING_LIBS)

main.o: main.c
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c main.c -o main.o

.PHONY: clean
clean:
	rm -f $(PROGRAM)
	rm -f ./*.o

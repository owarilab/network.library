PROGRAM = libqs_llm_module.a
PROGRAMOUT=../
CC      = gcc
AR      = ar
DEFINE  = -D_LINUX
CFLAG   = -pthread -Wall -O

# If you vendor llama.cpp under llm/third_party/llama.cpp, this default works.
LLAMA_CPP_DIR ?= ../third_party/llama.cpp
LLAMA_BUILD_DIR ?= $(LLAMA_CPP_DIR)/build
LLAMA_BIN_DIR ?= $(LLAMA_BUILD_DIR)/bin

LLAMA_ENABLE ?= 0
LLAMA_CUDA ?= 1

ifeq ($(LLAMA_ENABLE),1)
DEFINE  += -DQS_LLM_WITH_LLAMA_CPP=1
LLAMA_RUNTIME_TARGET = $(LLAMA_BIN_DIR)/libllama.so
LLAMA_CMAKE_ACCEL =
ifeq ($(LLAMA_CUDA),1)
LLAMA_CMAKE_ACCEL += -DGGML_CUDA=ON
endif
endif

# Embedding module: enabled with QS_EMBEDDING_MODULE_ENABLED=1
EMBEDDING_OBJS =

ifeq ($(QS_EMBEDDING_MODULE_ENABLED),1)
DEFINE  += -DQS_EMBEDDING_MODULE_ENABLED=1 -DQS_EMBEDDING_WITH_LLAMA_CPP=1
OBJGROUP += qs_embedding_module.o
EMBEDDING_OBJS += ../third_party/sqlite-vec/sqlite-vec.o
endif

INCDIR  = -I ../header -I ../../core/header -I $(LLAMA_CPP_DIR) -I $(LLAMA_CPP_DIR)/include -I $(LLAMA_CPP_DIR)/ggml/include
OBJGROUP += qs_llama_module.o

build: $(LLAMA_RUNTIME_TARGET) $(PROGRAM)

$(LLAMA_BIN_DIR)/libllama.so:
	cd $(LLAMA_CPP_DIR) && cmake -S . -B build -DBUILD_SHARED_LIBS=ON -DLLAMA_BUILD_TESTS=OFF -DLLAMA_BUILD_EXAMPLES=OFF $(LLAMA_CMAKE_ACCEL)
	cd $(LLAMA_CPP_DIR) && cmake --build build -j4

$(PROGRAM): $(OBJGROUP) $(EMBEDDING_OBJS)
	$(AR) r  $(PROGRAMOUT)$(PROGRAM) $(OBJGROUP) $(EMBEDDING_OBJS)

../third_party/sqlite-vec/sqlite-vec.o: ../third_party/sqlite-vec/sqlite-vec.c
	$(CC) $(CFLAG) -c $< -o $@

qs_llama_module.o: qs_llama_module.c
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c qs_llama_module.c -o qs_llama_module.o

qs_embedding_module.o: qs_embedding_module.c
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(PROGRAMOUT)$(PROGRAM)
	rm -f ./*.o

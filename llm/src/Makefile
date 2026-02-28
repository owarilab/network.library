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

INCDIR  = -I ../header -I ../../core/header -I $(LLAMA_CPP_DIR) -I $(LLAMA_CPP_DIR)/include -I $(LLAMA_CPP_DIR)/ggml/include
OBJGROUP = qs_llama_module.o

build: $(LLAMA_RUNTIME_TARGET) $(PROGRAM)

$(LLAMA_BIN_DIR)/libllama.so:
	cd $(LLAMA_CPP_DIR) && cmake -S . -B build -DBUILD_SHARED_LIBS=ON -DLLAMA_BUILD_TESTS=OFF -DLLAMA_BUILD_EXAMPLES=OFF $(LLAMA_CMAKE_ACCEL)
	cd $(LLAMA_CPP_DIR) && cmake --build build -j4

$(PROGRAM): $(OBJGROUP)
	$(AR) r  $(PROGRAMOUT)$(PROGRAM) $(OBJGROUP)

qs_llama_module.o: qs_llama_module.c
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c qs_llama_module.c -o qs_llama_module.o

.PHONY: clean
clean:
	rm -f $(PROGRAMOUT)$(PROGRAM)
	rm -f ./*.o

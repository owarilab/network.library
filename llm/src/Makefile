PROGRAM = libqs_llm_module.a
PROGRAMOUT=../
UNAME_S := $(shell uname -s)
CC      = $(if $(filter Darwin,$(UNAME_S)),clang,gcc)
AR      = ar
DEFINE  = $(if $(filter Darwin,$(UNAME_S)),,-D_LINUX)
CFLAG   = -pthread -Wall -O

# If you vendor llama.cpp under llm/third_party/llama.cpp, this default works.
LLAMA_CPP_DIR ?= ../third_party/llama.cpp
LLAMA_BUILD_DIR ?= $(LLAMA_CPP_DIR)/build
LLAMA_BIN_DIR ?= $(LLAMA_BUILD_DIR)/bin
LLAMA_BUILD_JOBS ?= $(shell if [ "$(UNAME_S)" = Darwin ]; then sysctl -n hw.ncpu; else nproc 2>/dev/null || printf 4; fi)
LLAMA_CMAKE_FLAGS = -DBUILD_SHARED_LIBS=ON -DLLAMA_BUILD_COMMON=OFF -DLLAMA_BUILD_TESTS=OFF -DLLAMA_BUILD_EXAMPLES=OFF -DLLAMA_BUILD_TOOLS=OFF -DLLAMA_BUILD_SERVER=OFF -DLLAMA_BUILD_APP=OFF -DLLAMA_BUILD_UI=OFF -DLLAMA_OPENSSL=OFF

LLAMA_ENABLE ?= 0
LLAMA_CUDA ?= 0
LLAMA_METAL ?= $(if $(filter Darwin,$(UNAME_S)),1,0)
LLAMA_CUDA_ARCH ?=

ifeq ($(LLAMA_ENABLE),1)
DEFINE  += -DQS_LLM_WITH_LLAMA_CPP=1
LLAMA_RUNTIME_TARGET = $(LLAMA_BIN_DIR)/$(if $(filter Darwin,$(UNAME_S)),libllama.dylib,libllama.so)
LLAMA_CMAKE_ACCEL =
ifeq ($(LLAMA_CUDA),1)
LLAMA_CMAKE_ACCEL += -DGGML_CUDA=ON
endif
ifeq ($(LLAMA_METAL),1)
LLAMA_CMAKE_ACCEL += -DGGML_METAL=ON
endif
ifneq ($(strip $(LLAMA_CUDA_ARCH)),)
LLAMA_CMAKE_ACCEL += -DCMAKE_CUDA_ARCHITECTURES=$(LLAMA_CUDA_ARCH)
endif
endif

# Always include the embedding object so its no-op API is available when the
# optional implementation is disabled.
EMBEDDING_OBJS = qs_embedding_module.o

ifeq ($(QS_EMBEDDING_MODULE_ENABLED),1)
DEFINE  += -DQS_EMBEDDING_MODULE_ENABLED=1 -DQS_EMBEDDING_WITH_LLAMA_CPP=1
EMBEDDING_OBJS += ../third_party/sqlite-vec/sqlite-vec.o
endif

INCDIR  = -I ../header -I ../../core/header -I $(LLAMA_CPP_DIR) -I $(LLAMA_CPP_DIR)/include -I $(LLAMA_CPP_DIR)/ggml/include
OBJGROUP += qs_llama_module.o

build: $(LLAMA_RUNTIME_TARGET) $(PROGRAM)

$(LLAMA_RUNTIME_TARGET):
	cd $(LLAMA_CPP_DIR) && cmake -S . -B $(notdir $(LLAMA_BUILD_DIR)) $(LLAMA_CMAKE_FLAGS) $(LLAMA_CMAKE_ACCEL)
	cd $(LLAMA_CPP_DIR) && cmake --build $(notdir $(LLAMA_BUILD_DIR)) -j$(LLAMA_BUILD_JOBS)

$(PROGRAM): $(OBJGROUP) $(EMBEDDING_OBJS)
	$(AR) r  $(PROGRAMOUT)$(PROGRAM) $(OBJGROUP) $(EMBEDDING_OBJS)

../third_party/sqlite-vec/sqlite-vec.o: ../third_party/sqlite-vec/sqlite-vec.c
	$(CC) $(CFLAG) -DSQLITE_CORE -c $< -o $@

qs_llama_module.o: qs_llama_module.c
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c qs_llama_module.c -o qs_llama_module.o

qs_embedding_module.o: qs_embedding_module.c
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(PROGRAMOUT)$(PROGRAM)
	rm -f ./*.o

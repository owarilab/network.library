PROGRAM = qs_llm_http_stream_server
TEST_PROGRAM = tests/test_llm_http_stream
UNAME_S := $(shell uname -s)
CC      = $(if $(filter Darwin,$(UNAME_S)),clang,gcc)
DEFINE  = $(if $(filter Darwin,$(UNAME_S)),,-D_LINUX)
CFLAG   = -pthread -Wall -O
TEST_CFLAG = -ffunction-sections -fdata-sections
LLAMA_ENABLE ?= 0
QS_EMBEDDING_MODULE_ENABLED ?= 0
LLAMA_CUDA ?= 0
LLAMA_METAL ?= $(if $(filter Darwin,$(UNAME_S)),1,0)
LLAMA_CPP_DIR ?= ../../llm/third_party/llama.cpp
LLAMA_BUILD_DIR ?= $(LLAMA_CPP_DIR)/build
LLAMA_LIB_DIR ?= $(LLAMA_BUILD_DIR)/bin
LLAMA_SHARED_LIB = $(if $(filter Darwin,$(UNAME_S)),libllama.dylib,libllama.so)

ifeq ($(LLAMA_ENABLE),1)
DEFINE += -DQS_LLM_WITH_LLAMA_CPP=1
LLAMA_LIBS = -L $(LLAMA_LIB_DIR) -Wl,-rpath,$(abspath $(LLAMA_LIB_DIR)) -lllama
LLAMA_RUNTIME_TARGET = $(LLAMA_LIB_DIR)/$(LLAMA_SHARED_LIB)
LLAMA_CMAKE_ACCEL =
ifeq ($(LLAMA_CUDA),1)
LLAMA_CMAKE_ACCEL += -DGGML_CUDA=ON
endif
ifeq ($(LLAMA_METAL),1)
LLAMA_CMAKE_ACCEL += -DGGML_METAL=ON
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

test: $(TEST_PROGRAM)
	./$(TEST_PROGRAM)

$(LLAMA_RUNTIME_TARGET):
	cd $(LLAMA_CPP_DIR) && cmake -S . -B build -DBUILD_SHARED_LIBS=ON -DLLAMA_BUILD_TESTS=OFF -DLLAMA_BUILD_EXAMPLES=OFF $(LLAMA_CMAKE_ACCEL)
	cd $(LLAMA_CPP_DIR) && cmake --build build -j4

../../core/libqs_core.a:
	cd ../../core/src && make

../../llm/libqs_llm_module.a: ../../llm/src/Makefile ../../llm/src/qs_llama_module.c ../../llm/src/qs_embedding_module.c
	cd ../../llm/src && make build LLAMA_ENABLE=$(LLAMA_ENABLE) QS_EMBEDDING_MODULE_ENABLED=$(QS_EMBEDDING_MODULE_ENABLED)

../../llm/libqs_embedding_module.a:
	cd ../../llm/src && make build LLAMA_ENABLE=$(LLAMA_ENABLE) QS_EMBEDDING_MODULE_ENABLED=$(QS_EMBEDDING_MODULE_ENABLED)

$(PROGRAM): $(OBJGROUP)
	$(CC) $(CFLAG) $(INCDIR) -o $(PROGRAM) $(OBJGROUP) $(LLAMA_LIBS) $(EMBEDDING_LIBS)

main.o: main.c
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c main.c -o main.o


$(TEST_PROGRAM): tests/test_llm_http_stream.c main.c ../../llm/src/qs_llama_module.c
	mkdir -p tests
	$(CC) $(DEFINE) $(CFLAG) $(TEST_CFLAG) $(INCDIR) -DQS_LLM_HTTP_STREAM_TESTING=1 -c tests/test_llm_http_stream.c -o tests/test_llm_http_stream.o
	$(CC) $(DEFINE) $(CFLAG) $(TEST_CFLAG) $(INCDIR) -DQS_LLM_HTTP_STREAM_TESTING=1 -Dmain=qs_llm_http_stream_app_main -c main.c -o tests/main_testable.o
	$(CC) $(DEFINE) $(CFLAG) $(TEST_CFLAG) $(INCDIR) -c ../../llm/src/qs_llama_module.c -o tests/qs_llama_module_test.o
	$(CC) $(CFLAG) tests/test_llm_http_stream.o tests/main_testable.o tests/qs_llama_module_test.o $(if $(filter Darwin,$(UNAME_S)),,-Wl,--gc-sections) -o $(TEST_PROGRAM)

.PHONY: clean
clean:
	rm -f $(PROGRAM)
	rm -f $(TEST_PROGRAM)
	rm -f tests/*.o
	rm -f ./*.o

.PHONY: test

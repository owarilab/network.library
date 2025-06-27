# Combined static library Makefile
LIBNAME = libqs.a
CC      = gcc
AR      = ar
ARFLAGS = rcs
DEFINE  = -D_LINUX
CFLAG   = -pthread -Wall -O

# Source libraries
CORE_LIB = ../../core/libqs_core.a
SSL_LIB  = ../../ssl/libqs_ssl_module.a

# Temporary directory for extracting object files
TEMP_DIR = temp_objs

build: $(LIBNAME)

# Build prerequisite libraries
$(CORE_LIB):
	cd ../../core/src && make

$(SSL_LIB):
	cd ../../ssl/src && make

# Create combined static library
$(LIBNAME): $(CORE_LIB) $(SSL_LIB)
	@echo "Creating combined static library $(LIBNAME)..."
	@mkdir -p $(TEMP_DIR)
	@cd $(TEMP_DIR) && ar x ../$(CORE_LIB)
	@cd $(TEMP_DIR) && ar x ../$(SSL_LIB)
	$(AR) $(ARFLAGS) $(LIBNAME) $(TEMP_DIR)/*.o
	@rm -rf $(TEMP_DIR)
	@echo "Combined library $(LIBNAME) created successfully"

.PHONY: clean all
clean: 
	cd ../../core/src && make clean
	cd ../../ssl/src && make clean
	rm -f $(LIBNAME)
	rm -rf $(TEMP_DIR)

all: clean build

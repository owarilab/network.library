PROGRAM=libqs_core.a
PROGRAMOUT=../
CC=gcc
AR=ar
INCDIR = -I ../header

# default _LINUX
#CFLAG = $(DEFINE) $(INCDIR) -Wall -O0 -pg
ifdef _LINUX_SHARED
CFLAG = $(DEFINE) $(INCDIR) -Wall -O0 -fPIC
else
CFLAG = $(DEFINE) $(INCDIR) -Wall -O0
endif
#CFLAG = -D_BSD_UNIX $(INCDIR) -Wall -O
#CFLAG = -D_LINUX $(INCDIR) -Wall -O
#CFLAG = -D_WINDOWS $(INCDIR) -Wall -O
SOURCES=$(wildcard *.c)
OBJGROUP    = $(SOURCES:.c=.o)

all: $(PROGRAM)

$(PROGRAM): $(OBJGROUP)
	$(AR) r $(PROGRAMOUT)$(PROGRAM) $(OBJGROUP)

.c.o:
	$(CC) $(CFLAG) -c $<

.PHONY: clean
clean:
	rm -f $(PROGRAMOUT)$(PROGRAM)
	rm -f ./*.o


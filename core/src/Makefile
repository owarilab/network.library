PROGRAM=libgdt_core.a
PROGRAMOUT=../
CC=gcc
AR=ar
INCDIR = -I ../header

# default _LINUX
CFLAG = $(DEFINE) $(INCDIR) -pthread -Wall -O
#CFLAG = -D_BSD_UNIX $(INCDIR) -pthread -Wall -O
#CFLAG = -D_LINUX $(INCDIR) -pthread -Wall -O
#CFLAG = -D_WINDOWS $(INCDIR) -pthread -Wall -O
SOURCES=$(wildcard *.c)
OBJGROUP    = $(SOURCES:.c=.o)

all: $(PROGRAM)

$(PROGRAM): $(OBJGROUP)
	$(AR) r $(PROGRAMOUT)$(PROGRAM) $(OBJGROUP)

.c.o:
	$(CC) $(CFLAG) -c $<

.PHONY: clean
clean:
	\rm -f $(PROGRAMOUT)$(PROGRAM)
	\rm -f ./*.o


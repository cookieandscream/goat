TARGETS = libgoat.a

APIH = $(wildcard include/*.h)
SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:.c=.o)

CFLAGS += -g -Wall -Wno-unknown-pragmas -Isrc -Iinclude

.PHONY : all clean realclean

all : $(TARGETS)

$(OBJS) : $(APIH)

libgoat.a : $(OBJS)
	rm -f $@
	ar cq $@ $(OBJS)

clean :
	rm -f $(OBJS) $(TARGETS) core

realclean : clean

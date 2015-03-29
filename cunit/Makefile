TARGET=cunit

all: $(TARGET)

CFLAGS=-Wall -I../src
LDFLAGS=-L../.libs
LDLIBS=-lgoat -lcunit

$(TARGET): cunit.o cunit-suites-init.o message/suite.o message/constructor.o

cunit-suites-init.c: cunit.pl
	./cunit.pl > $@

clean:
	$(RM) -f cunit-suites-init.c *.o message/*.o


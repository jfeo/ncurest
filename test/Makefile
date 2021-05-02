CC=gcc
CFLAGS=-std=gnu11 -pedantic
LDLIBS=-lcheck

all: ../bin/tests

../bin/tests: ../obj/tests.o ../obj/http.o
	@mkdir -p ../bin
	$(CC) $(CFLAGS) $(LDLIBS) -o ../bin/tests ../obj/tests.o ../obj/http.o

../obj/tests.o: tests.c
	@mkdir -p ../obj
	$(CC) $(CFLAGS) $(LDLIBS) -c ../test/tests.c -o ../obj/tests.o

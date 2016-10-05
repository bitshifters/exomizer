#
# Makefile for exomizer decruncher
#

#CFLAGS = -Wall -O3 -ffast-math -mtune=i686 -fomit-frame-pointer -fgcse -pedantic
#LDFLAGS = -s

CFLAGS = -g -mtune=i686 -Wall
LDFLAGS = -g -mtune=i686

TEST_OBJS = main.o exodecrunch.o
TEST2_OBJS = main2.o exodecr.o

#.SILENT:

.PHONY: build

build: $(MAKEFILE) test test2

test: main test1.exo test2.exo test3.exo
	./main test1.exo test1.dec
	cmp -l test1.bin test1.dec
	./main test2.exo test2.dec
	cmp -l test2.bin test2.dec
	./main test3.exo test3.dec
	cmp -l test3.bin test3.dec

test2: main2 test1.oxe test2.oxe test3.oxe
	./main2 test1.oxe test1.dec
	cmp -l test1.bin test1.dec
	./main2 test2.oxe test2.dec
	cmp -l test2.bin test2.dec
	./main2 test3.oxe test3.dec
	cmp -l test3.bin test3.dec

main: deps $(TEST_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(TEST_OBJS)

main2: deps $(TEST2_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(TEST2_OBJS)

clean:
	-$(RM) $(TEST_OBJS) $(TEST2_OBJS) main main.exe main2 main2.exe \
	*.o deps *.exo *.oxe *.dec

-include deps

deps: $(wildcard *.h)
	$(CC) -MM $(wildcard *.c) >$@

%.exo: %.bin
	../src/exomizer raw $< -o $@

%.oxe: %.bin
	../src/exomizer raw -b $< -o $@

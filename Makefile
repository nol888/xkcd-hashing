CC=gcc
LD=gcc
CFLAGS=-O3 -march=native -std=c99 -ggdb
LDFLAGS=

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<
%.o: %.s
	$(CC) -c $(CFLAGS) -o $@ $<

all: skein/skein.o skein/skein_block.o xkcd.o
	$(LD) $(LDFLAGS) -o xkcd $^

profile : CFLAGS=-O3 -march=native -std=c99 -ggdb -pg
profile : LDFLAGS=-pg
profile : clean all

clean:
	-rm xkcd *.o skein/*.o

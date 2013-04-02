CC=gcc
LD=gcc
CFLAGS=-O3 -mpopcnt -std=c99 -ggdb
LDFLAGS=

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<
%.o: %.s
	$(CC) -c $(CFLAGS) -o $@ $<

all: skein.o skein_block.o xkcd.o
	$(LD) $(LDFLAGS) -o xkcd $^

profile : CFLAGS=-O3 -mpopcnt -std=c99 -ggdb -pg
profile : LDFLAGS=-pg
profile : all

verbose : CFLAGS=-O3 -mpopcnt -std=c99 -ggdb -DSHOW_STATUS
verbose : all

clean:
	rm xkcd *.o

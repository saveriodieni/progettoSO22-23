CC=gcc
CCOPTS=--std=gnu99 -Wall -D_LIST_DEBUG_ -g
AR=ar

BINS=randomizer

#disastros_test

.phony: clean all


all:	$(BINS)

%.o:	%.c $(HEADERS)
	$(CC) $(CCOPTS) -c -o $@  $<

randomizer:	randomizer.c 
	$(CC) $(CCOPTS) -o $@ $^

clean:
	rm -rf *.o *~  $(BINS)

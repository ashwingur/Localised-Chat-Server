CC=gcc
CFLAGS=-Wall -Werror -Wvla -std=gnu11 -fsanitize=address
PFLAGS=-fprofile-arcs -ftest-coverage
DFLAGS=-g
HEADERS=server.h
SRC=server.c

.PHONY: test

procchat: $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(DFLAGS) $(SRC) -o $@

test:
	$(CC) $(CFLAGS) $(PFLAGS) $(SRC) -o $@
	bash test.sh

clean:
	rm -f procchat
	rm -f server.c.gcov
	rm -f server.gcda
	rm -f server.gcno
	rm -f test
	find . -type p -delete
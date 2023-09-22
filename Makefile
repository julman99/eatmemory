all: args/args.c eatmemory.c

ifeq ($(shell uname 2>/dev/null), AIX)
	gcc -maix64 args/args.c eatmemory.c -o eatmemory
else
	$(CC) args/args.c eatmemory.c -o eatmemory
endif


install: eatmemory
	mkdir -p $(PREFIX)/bin
	install -m 0755 eatmemory $(PREFIX)/bin/

clean:
	rm -rf *o eatmemory

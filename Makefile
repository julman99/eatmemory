all: eatmemory.c
	$(CC) eatmemory.c -o eatmemory

install: eatmemory
	install -D -m 0755 eatmemory $(PREFIX)/bin/

clean:
	rm -rf *o eatmemory

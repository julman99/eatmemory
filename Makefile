all: eatmemory.c
	$(CC) eatmemory.c -o eatmemory

install: eatmemory
	install -m 0755 eatmemory $(PREFIX)/usr/bin/eatmemory

clean:
	rm -rf *o eatmemory

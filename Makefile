all: eatmemory.c
	$(CC) eatmemory.c -o eatmemory

install: eatmemory
	mkdir -p $(PREFIX)/bin
	install -m 0755 eatmemory $(PREFIX)/bin/

clean:
	rm -rf *o eatmemory

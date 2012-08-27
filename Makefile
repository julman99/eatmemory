all: eatmemory.c
	$(CC) eatmemory.c -o eatmemory

install: eatmemory
	install -m 0755 eatmemory $(prefix)/bin/eatmemory


clean:
	rm -rf *o eatmemory

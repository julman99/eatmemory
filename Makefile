all: eatmemory.c
	$(CC) eatmemory.c -o eatmemory

install: eatmemory
	rm -f /bin/eatmemory
	rm -f $(prefix)/bin/eatmemory
	install -m 0755 eatmemory $(prefix)/usr/bin/eatmemory


clean:
	rm -rf *o eatmemory

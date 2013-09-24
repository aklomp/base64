CFLAGS += -std=c89 -O3 -march=native -Wall -Wextra -pedantic

test: test.c ../base64.c
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: clean

clean:
	rm -f test

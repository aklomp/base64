CFLAGS += -std=c89 -O3 -march=native -Wall -Wextra -pedantic

base64: main.c base64.c
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: clean

clean:
	rm -f base64

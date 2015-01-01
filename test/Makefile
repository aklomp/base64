CFLAGS += -std=c89 -O3 -Wall -Wextra -pedantic

.PHONY: clean test

test_base64: test_base64.c ../base64.c
	$(CC) $(CFLAGS) -o $@ $^

test: clean test_base64
	./test_base64

clean:
	rm -f test_base64

CFLAGS += -std=c89 -O3 -Wall -Wextra -pedantic

.PHONY: clean test

test: clean test_base64
	./test_base64

test_base64: test_base64.c ../lib/libbase64.a
	$(CC) $(CFLAGS) -o $@ $^

../%:
	make -C .. $*

clean:
	rm -f test_base64

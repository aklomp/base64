CFLAGS += -std=c89 -O3 -Wall -Wextra -pedantic

.PHONY: clean test

test: clean test_base64
	./test_base64

test_base64: test_base64.c ../libbase64.a
	$(CC) $(CFLAGS) -o $@ $^

../libbase64.a:
	make -C .. libbase64.a

clean:
	rm -f test_base64

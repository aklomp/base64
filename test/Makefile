CFLAGS += -O3 -Wall -Wextra -pedantic

.PHONY: clean test

test: clean test_base64
	./test_base64

test_base64: test_base64.c ../lib/libbase64.a moby_dick_plain.o moby_dick_base64.o
	$(CC) $(CFLAGS) -o $@ $^

../%:
	make -C .. $*

# Create object files from these test vectors:
moby_dick_plain.o: moby_dick_plain.txt
	$(LD) --relocatable --format=binary $^ -o $@

moby_dick_base64.o: moby_dick_base64.txt
	$(LD) --relocatable --format=binary $^ -o $@

clean:
	rm -f test_base64 *.o

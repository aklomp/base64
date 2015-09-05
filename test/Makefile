CFLAGS += -std=c99 -O3 -Wall -Wextra -pedantic

.PHONY: clean test

test: clean test_base64 benchmark
	./test_base64
	./benchmark

test_base64: test_base64.c codec_supported.o ../lib/libbase64.o moby_dick_plain.o moby_dick_base64.o
	$(CC) $(CFLAGS) -o $@ $^

benchmark: benchmark.c codec_supported.o ../lib/libbase64.o
	$(CC) $(CFLAGS) -o $@ $^ -lrt

../%:
	make -C .. $*

# Create object files from these test vectors:
moby_dick_plain.o: moby_dick_plain.txt
	$(LD) --relocatable --format=binary -o $@ $^

moby_dick_base64.o: moby_dick_base64.txt
	$(LD) --relocatable --format=binary -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f benchmark test_base64 *.o

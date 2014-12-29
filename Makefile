CFLAGS += -std=c89 -O3 -Wall -Wextra -pedantic

base64: main.c base64.c
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: clean analyze

analyze: clean
	scan-build --use-analyzer=`which clang` --status-bugs make

clean:
	rm -f base64

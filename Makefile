LIBTOOL = libtool
UNAME_S = $(shell uname -s)

CFLAGS += -std=c89 -O3 -Wall -Wextra -pedantic

OBJS = base64.o

.PHONY: all analyze clean

all: base64 libbase64.a

base64: main.o libbase64.a
	$(CC) $(CFLAGS) -o $@ $^

libbase64.a: $(OBJS)
ifeq ($(UNAME_S), Darwin)
	$(LIBTOOL) -static $(OBJS) -o $@
else
	$(AR) -r $@ $(OBJS)
endif

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $^

analyze: clean
	scan-build --use-analyzer=`which clang` --status-bugs make

clean:
	rm -f base64 libbase64.a main.o $(OBJS)

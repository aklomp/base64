LIBTOOL = libtool
UNAME_S = $(shell uname -s)

CFLAGS += -std=c89 -O3 -Wall -Wextra -pedantic

OBJS = \
  lib/libbase64.o \
  lib/codec_choose.o \
  lib/codec_plain.o \
  lib/codec_ssse3.o

.PHONY: all analyze clean

all: bin/base64 lib/libbase64.a

bin/base64: bin/base64.o lib/libbase64.a
	$(CC) $(CFLAGS) -o $@ $^

lib/libbase64.a: $(OBJS)
ifeq ($(UNAME_S), Darwin)
	$(LIBTOOL) -static $(OBJS) -o $@
else
	$(AR) -r $@ $(OBJS)
endif

lib/codec_ssse3.o: CFLAGS += -mssse3

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $^

analyze: clean
	scan-build --use-analyzer=`which clang` --status-bugs make

clean:
	rm -f bin/base64 lib/libbase64.a $(OBJS)

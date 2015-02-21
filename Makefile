LIBTOOL = libtool
UNAME_S = $(shell uname -s)

CFLAGS += -std=c89 -O3 -Wall -Wextra -pedantic

OBJS = \
  lib/libbase64.o \
  lib/codec_avx2.o \
  lib/codec_choose.o \
  lib/codec_neon.o \
  lib/codec_plain.o \
  lib/codec_ssse3.o

# Compile-time feature detection;
# specifying unavailable architecture flags causes compiler errors:
HAVE_AVX2  ?= $(shell echo 'int main;' | $(CC) -mavx2     -Werror -o /dev/null -x c - >/dev/null 2>&1 && echo 1 || echo 0)
HAVE_NEON  ?= $(shell echo 'int main;' | $(CC) -mfpu=neon -Werror -o /dev/null -x c - >/dev/null 2>&1 && echo 1 || echo 0)
HAVE_SSSE3 ?= $(shell echo 'int main;' | $(CC) -mssse3    -Werror -o /dev/null -x c - >/dev/null 2>&1 && echo 1 || echo 0)

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

lib/config.h:
	@echo "#define HAVE_AVX2  $(HAVE_AVX2)"   > $@
	@echo "#define HAVE_NEON  $(HAVE_NEON)"  >> $@
	@echo "#define HAVE_SSSE3 $(HAVE_SSSE3)" >> $@

lib/codec_choose.o: lib/config.h

ifeq ($(HAVE_AVX2), 1)
lib/codec_avx2.o: CFLAGS += -mavx2
endif

ifeq ($(HAVE_NEON), 1)
lib/codec_neon.o: CFLAGS += -mfpu=neon
endif

ifeq ($(HAVE_SSSE3), 1)
lib/codec_ssse3.o: CFLAGS += -mssse3
endif

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

analyze: clean
	scan-build --use-analyzer=`which clang` --status-bugs make

clean:
	rm -f bin/base64 lib/libbase64.a lib/config.h $(OBJS)

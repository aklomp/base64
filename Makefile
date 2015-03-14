LIBTOOL = libtool
UNAME_S = $(shell uname -s)

CFLAGS += -O3 -Wall -Wextra -pedantic

OBJS = \
  lib/libbase64.o \
  lib/codec_avx2.o \
  lib/codec_choose.o \
  lib/codec_neon32.o \
  lib/codec_neon64.o \
  lib/codec_plain.o \
  lib/codec_ssse3.o

HAVE_AVX2   = 0
HAVE_NEON32 = 0
HAVE_NEON64 = 0
HAVE_SSSE3  = 0

# The user should supply compiler flags for the codecs they want to build.
# Check which codecs we're going to include:
ifdef AVX2_CFLAGS
  HAVE_AVX2 = 1
endif
ifdef NEON32_CFLAGS
  HAVE_NEON32 = 1
endif
ifdef NEON64_CFLAGS
  HAVE_NEON64 = 1
endif
ifdef SSSE3_CFLAGS
  HAVE_SSSE3 = 1
endif

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
	@echo "#define HAVE_AVX2   $(HAVE_AVX2)"    > $@
	@echo "#define HAVE_NEON32 $(HAVE_NEON32)" >> $@
	@echo "#define HAVE_NEON64 $(HAVE_NEON64)" >> $@
	@echo "#define HAVE_SSSE3  $(HAVE_SSSE3)"  >> $@

lib/codec_choose.o: lib/config.h

lib/codec_avx2.o:   CFLAGS += $(AVX2_CFLAGS)
lib/codec_neon32.o: CFLAGS += $(NEON32_CFLAGS)
lib/codec_neon64.o: CFLAGS += $(NEON64_CFLAGS)
lib/codec_ssse3.o:  CFLAGS += $(SSSE3_CFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

analyze: clean
	scan-build --use-analyzer=`which clang` --status-bugs make

clean:
	rm -f bin/base64 lib/libbase64.a lib/config.h $(OBJS)

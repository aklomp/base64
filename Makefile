CFLAGS += -std=c99 -O3 -Wall -Wextra -pedantic

# Set OBJCOPY if not defined by environment:
OBJCOPY ?= objcopy

OBJS = \
  lib/arch/avx2/codec.o \
  lib/arch/generic/codec.o \
  lib/arch/neon32/codec.o \
  lib/arch/neon64/codec.o \
  lib/arch/ssse3/codec.o \
  lib/arch/sse41/codec.o \
  lib/arch/sse42/codec.o \
  lib/arch/avx/codec.o \
  lib/lib.o \
  lib/codec_choose.o

HAVE_AVX2   = 0
HAVE_NEON32 = 0
HAVE_NEON64 = 0
HAVE_SSSE3  = 0
HAVE_SSE41  = 0
HAVE_SSE42  = 0
HAVE_AVX    = 0
HAVE_FAST_UNALIGNED_ACCESS = 0

# The user should supply compiler flags for the codecs they want to build.
# Check which codecs we're going to include:
ifdef AVX2_CFLAGS
  HAVE_AVX2 = 1
  HAVE_FAST_UNALIGNED_ACCESS = 1
endif
ifdef NEON32_CFLAGS
  HAVE_NEON32 = 1
endif
ifdef NEON64_CFLAGS
  HAVE_NEON64 = 1
endif
ifdef SSSE3_CFLAGS
  HAVE_SSSE3 = 1
  HAVE_FAST_UNALIGNED_ACCESS = 1
endif
ifdef SSE41_CFLAGS
  HAVE_SSE41 = 1
  HAVE_FAST_UNALIGNED_ACCESS = 1
endif
ifdef SSE42_CFLAGS
  HAVE_SSE42 = 1
  HAVE_FAST_UNALIGNED_ACCESS = 1
endif
ifdef AVX_CFLAGS
  HAVE_AVX = 1
  HAVE_FAST_UNALIGNED_ACCESS = 1
endif
ifdef OPENMP
  CFLAGS += -fopenmp
endif


.PHONY: all analyze clean

all: bin/base64 lib/libbase64.o

bin/base64: bin/base64.o lib/libbase64.o
	$(CC) $(CFLAGS) -o $@ $^

lib/libbase64.o: $(OBJS)
	$(LD) -r -o $@ $^
	$(OBJCOPY) --keep-global-symbols=lib/exports.txt $@

lib/config.h:
	@echo "#define HAVE_AVX2                  $(HAVE_AVX2)"                   > $@
	@echo "#define HAVE_NEON32                $(HAVE_NEON32)"                >> $@
	@echo "#define HAVE_NEON64                $(HAVE_NEON64)"                >> $@
	@echo "#define HAVE_SSSE3                 $(HAVE_SSSE3)"                 >> $@
	@echo "#define HAVE_SSE41                 $(HAVE_SSE41)"                 >> $@
	@echo "#define HAVE_SSE42                 $(HAVE_SSE42)"                 >> $@
	@echo "#define HAVE_AVX                   $(HAVE_AVX)"                   >> $@
	@echo "#define HAVE_FAST_UNALIGNED_ACCESS $(HAVE_FAST_UNALIGNED_ACCESS)" >> $@

lib/table_generator: lib/table_generator.o
	$(CC) $(CFLAGS) -o $@ $^

lib/tables.h: lib/table_generator
	./lib/table_generator > $@

$(OBJS): lib/config.h

lib/lib.o: lib/tables.h

lib/arch/avx2/codec.o:   CFLAGS += $(AVX2_CFLAGS)
lib/arch/neon32/codec.o: CFLAGS += $(NEON32_CFLAGS)
lib/arch/neon64/codec.o: CFLAGS += $(NEON64_CFLAGS)
lib/arch/ssse3/codec.o:  CFLAGS += $(SSSE3_CFLAGS)
lib/arch/sse41/codec.o:  CFLAGS += $(SSE41_CFLAGS)
lib/arch/sse42/codec.o:  CFLAGS += $(SSE42_CFLAGS)
lib/arch/avx/codec.o:    CFLAGS += $(AVX_CFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

analyze: clean
	scan-build --use-analyzer=`which clang` --status-bugs make

clean:
	rm -f bin/base64 bin/base64.o lib/libbase64.o lib/table_generator.o lib/table_generator lib/tables.h lib/config.h $(OBJS)

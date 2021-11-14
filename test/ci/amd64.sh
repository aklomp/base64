#!/bin/bash
set -ve

export SSSE3_CFLAGS=-mssse3
export SSE41_CFLAGS=-msse4.1
export SSE42_CFLAGS=-msse4.2
export AVX_CFLAGS=-mavx
# no AVX2 on GHA macOS
if [ "$(uname -s)" != "Darwin" ]; then
	export AVX2_CFLAGS=-mavx2
fi

if [ "${OPENMP:-}" == "0" ]; then
	unset OPENMP
fi

uname -a
${CC} --version

make
make -C test

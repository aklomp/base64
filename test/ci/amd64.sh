#!/bin/bash
set -ve

export SSSE3_CFLAGS=-mssse3
export SSE41_CFLAGS=-msse4.1
export SSE42_CFLAGS=-msse4.2
export AVX_CFLAGS=-mavx
export AVX2_CFLAGS=-mavx2

uname -a
${TRAVIS_COMPILER} --version

make
make -C test

#!/bin/bash
set -ve

MACHINE=$(uname -m)
if [ "${MACHINE}" == "x86_64" ]; then
	export SSSE3_CFLAGS=-mssse3
	export SSE41_CFLAGS=-msse4.1
	export SSE42_CFLAGS=-msse4.2
	export AVX_CFLAGS=-mavx
	export AVX2_CFLAGS=-mavx2
	export AVX512_CFLAGS="-mavx512vl -mavx512vbmi"
	# Temporarily disable AVX512; it is not available in CI yet.
	export BASE64_TEST_SKIP_AVX512=1
elif [ "${MACHINE}" == "aarch64" ]; then
	export NEON64_CFLAGS="-march=armv8-a"
elif [ "${MACHINE}" == "armv7l" ]; then
	export NEON32_CFLAGS="-march=armv7-a -mfloat-abi=hard -mfpu=neon"
fi

if [ "${OPENMP:-}" == "0" ]; then
	unset OPENMP
fi

uname -a
${CC} --version

make
make -C test

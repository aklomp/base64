#!/bin/bash
set -ve

uname -a
${TRAVIS_COMPILER} --version

make
make -C test

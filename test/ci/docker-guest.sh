#!/bin/bash
set -ve

apt-get -y update
apt-get -y install ${TRAVIS_COMPILER} make

uname -a
${TRAVIS_COMPILER} --version

make
make -C test

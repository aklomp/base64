#!/bin/bash
set -ve

# Enable multi-arch support on the Docker host:
docker run --rm --privileged \
  multiarch/qemu-user-static:register --reset

# Run the build in a PowerPC Docker container:
docker run --rm -t -v $PWD:/build -w /build \
  --env TRAVIS_COMPILER="${TRAVIS_COMPILER}" \
  multiarch/ubuntu-debootstrap:powerpc-xenial \
  test/ci/docker-guest.sh

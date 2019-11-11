#!/bin/bash
set -ve

# Enable multi-arch support on the Docker host:
docker run --rm --privileged \
  multiarch/qemu-user-static:register --reset

# Run the build in an ARM32 Docker container:
docker run --rm -t -v $PWD:/build -w /build \
  --env NEON32_CFLAGS="-march=armv7-a -mfloat-abi=hard -mfpu=neon" \
  --env TRAVIS_COMPILER="${TRAVIS_COMPILER}" \
  multiarch/debian-debootstrap:armhf-jessie \
  test/ci/docker-guest.sh

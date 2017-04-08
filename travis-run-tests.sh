#!/bin/bash

set -ev

if [ -n "$TRAVIS_TAG" ]; then
  docker build -t goloschain/golos:"${TRAVIS_TAG}" .
else
  docker build -t goloschain/golos .
fi

docker images


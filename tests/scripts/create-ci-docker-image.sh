#!/bin/bash
set -x

DOCKER_CACHE_DIR="$HOME/docker"

if [[ ! -d $DOCKER_CACHE_DIR ]]; then
    mkdir -p $DOCKER_CACHE_DIR
fi

if [[ -e $DOCKER_CACHE_DIR/image.tar ]]; then
    du -sh $DOCKER_CACHE_DIR/image.tar
    docker load -i $DOCKER_CACHE_DIR/image.tar
else
    docker build --rm=false \
        -t steemitinc/ci-test-environment:latest \
        -f tests/scripts/Dockerfile.testenv . && \
    mkdir -p ~/docker && \
    docker save steemitinc/ci-test-environment:latest \
        > $DOCKER_CACHE_DIR/image.tar.tmp && \
    mv $DOCKER_CACHE_DIR/image.tar.tmp $DOCKER_CACHE_DIR/image.tar && \
    du -sh $DOCKER_CACHE_DIR/image.tar
fi

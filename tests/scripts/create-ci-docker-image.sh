#!/bin/bash
set -x

# exit if we're not running on circleci:
if [[ -z "$CIRCLE_BUILD_NUM" ]]; then
    echo "This is to be run on CircleCI only." > /dev/stderr
    exit 1
fi

DOCKER_CACHE_DIR="$HOME/docker"

if [[ ! -d $DOCKER_CACHE_DIR ]]; then
    mkdir -p $DOCKER_CACHE_DIR
fi

if [[ -e $DOCKER_CACHE_DIR/image.tar ]]; then
    du -sh $DOCKER_CACHE_DIR/image.tar
    docker load -i $DOCKER_CACHE_DIR/image.tar
else
    make docker_build_container && \
    mkdir -p ~/docker && \
    docker save steemitinc/ci-test-environment:latest \
        > $DOCKER_CACHE_DIR/image.tar.tmp && \
    mv $DOCKER_CACHE_DIR/image.tar.tmp $DOCKER_CACHE_DIR/image.tar && \
    du -sh $DOCKER_CACHE_DIR/image.tar
fi

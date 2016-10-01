#!/bin/bash

set -x

PKGS="docker docker-machine"

# install packages with homebrew if they don't exist
for PKG in $PKGS ; do
    if ! which $PKG 2>&1 >/dev/null ; then
        brew install $PKG
    fi
done


if ! docker ps 2>&1 >/dev/null ; then
    TOTAL_MEMORY_BYTES=$(sysctl hw.memsize | awk '{print $2}')
    TOTAL_MEMORY_MEGS=$(( $TOTAL_MEMORY_BYTES / 1048576 ))
    HALF_MEMORY_MEGS=$(( $TOTAL_MEMORY_MEGS / 2 ))

    MACHINE_NAME=steemtest
    CREATED=1
    docker-machine create \
        --driver virtualbox \
        --virtualbox-no-share \
        --virtualbox-cpu-count "-1" \
        --virtualbox-memory $HALF_MEMORY_MEGS \
        $MACHINE_NAME

    eval $(docker-machine env $MACHINE_NAME)
fi

make docker

if [[ $CREATED -eq 1 ]]; then
    docker-machine rm -f $MACHINE_NAME
fi

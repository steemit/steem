# Automated Testing Documentation

## To Run The Tests

In the root of the repository.

    docker build --rm=false \
        -t golosd/golosd-test \
        -f share/golosd/docker/Dockerfile-test .

## To Troubleshoot Failing Tests

    docker run -ti \
        golosd/golosd-test \
        /bin/bash

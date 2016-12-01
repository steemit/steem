#!/bin/bash
export IMAGE_NAME="steemit/steem:${GIT_BRANCH#*/}"
sudo docker build -t=$IMAGE_NAME .
sudo docker login --username=$DOCKER_USER --password=$DOCKER_PASS
sudo docker push $IMAGE_NAME
sudo rm -rf /tmp/ramdisk/*

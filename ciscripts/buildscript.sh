#!/bin/bash
set -e
export IMAGE_NAME="steemit/steem:$BRANCH_NAME"
if [[ $IMAGE_NAME == "steemit/steem:stable" ]] ; then
  IMAGE_NAME="steemit/steem:latest"
fi
sudo docker build --build-arg BUILD_STEP=2 -t=$IMAGE_NAME .
sudo docker login --username=$DOCKER_USER --password=$DOCKER_PASS
sudo docker push $IMAGE_NAME
sudo docker run -v /var/jenkins_home:/var/jenkins $IMAGE_NAME cp -r /var/cobertura /var/jenkins
cp -r /var/jenkins_home/cobertura .

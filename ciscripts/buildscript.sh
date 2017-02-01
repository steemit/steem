#!/bin/bash
set -e
export IMAGE_NAME="steemit/steem:${GIT_BRANCH#*/}"
if [[ $IMAGE_NAME == "steemit/steem:stable" ]] ; then
  IMAGE_NAME="steemit/steem:latest"
fi
# switch workspace into directory used for branch
mkdir ${GIT_BRANCH#*/}
cp -a temp-repo-folder/. ${GIT_BRANCH#*/}
rm -rf temp-repo-folder/*
cd ${GIT_BRANCH#*/}
sudo docker build -t=$IMAGE_NAME .
sudo docker login --username=$DOCKER_USER --password=$DOCKER_PASS
sudo docker push $IMAGE_NAME
sudo docker run -v /var/jenkins_home:/var/jenkins $IMAGE_NAME cp -r /var/cobertura /var/jenkins
cp -r /var/jenkins_home/cobertura .

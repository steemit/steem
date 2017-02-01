#!/bin/bash
set -e
export IMAGE_NAME="steemit/steem:${GIT_BRANCH#*/}"
if [[ $IMAGE_NAME == "steemit/steem:stable" ]] ; then
  IMAGE_NAME="steemit/steem:latest"
fi
# switch workspace into directory used for branch
export BUILD_SPACE=`mktemp -d`
cp -a temp-repo-folder/. $BUILD_SPACE
rm -rf temp-repo-folder/*
cd $BUILD_SPACE
curl -XPOST -H "Authorization: token $GITHUB_SECRET" https://api.github.com/repos/steemit/steem/statuses/$(git rev-parse HEAD) -d "{
  \"state\": \"pending\",
  \"target_url\": \"${BUILD_URL}\",
  \"description\": \"The build is now pending in jenkinsci!\",
  \"context\": \"jenkins-ci-steemit\"
}"
sudo docker build -t=$IMAGE_NAME .
sudo docker login --username=$DOCKER_USER --password=$DOCKER_PASS
sudo docker push $IMAGE_NAME
sudo docker run -v /var/jenkins_home:/var/jenkins $IMAGE_NAME cp -r /var/cobertura /var/jenkins
cp -r /var/jenkins_home/cobertura .

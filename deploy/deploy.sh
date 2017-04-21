#!/bin/bash

#set -xe

  deploy/secret_test.sh $1 $2
  
  exit 1

if [ $TRAVIS_BRANCH == "master" ]; then

  deploy/set-ssh.sh
  deploy/docs-deploy.sh --yes

else
  echo "Nothing to deploy, since the current branch is not master."
fi


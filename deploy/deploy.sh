#!/bin/bash

set -xe

if [ $TRAVIS_BRANCH == "master" ]; then

  ./set-ssh.sh
  ./docs-deploy.sh --yes

else
  echo "Nothing to deploy, since the current branch is not master."
fi


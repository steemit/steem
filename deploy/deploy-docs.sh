#!/bin/bash

set -x

if [ $TRAVIS_BRANCH == "master" ]; then
    
  openssl aes-256-cbc -K $encrypted_2974970c26d0_key -iv $encrypted_2974970c26d0_iv -in deploy-key.enc -out deploy-key -d

  rm deploy-key.enc
  chmod 600 deploy-key
  mv deploy-key ~/.ssh/id_rsa

  doxygen
  cd documentation/doxygen/html
  git init

  git remote add deploy "deploy@developers.golos.io:/www"
  git config user.name "Travis CI"
  git config user.email "travis@travis-ci.org"

  git add .
  git commit -m "Deploy"
  git push --force deploy master
else
  echo "Not deploying, since this branch isn't master."
fi


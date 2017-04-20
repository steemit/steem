#!/bin/bash

set -xe

if [ -n $TRAVIS_BRANCH ]; then

  cd deploy/

  openssl aes-256-cbc -K $encrypted_87a773837d6d_key -iv $encrypted_87a773837d6d_iv -in deploy-key.enc -out deploy-key -d

  rm deploy-key.enc

  chmod 600 deploy-key
  mv deploy-key ~/.ssh/id_rsa

  chmod 600 config
  mv config ~/.ssh/config

  cd ..

else

  echo "Only for Travis-CI !!!"

fi


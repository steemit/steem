#!/bin/bash
set -e
sudo docker build --build-arg BUILD_STEP=1 -t=steemit/steem:tests .
sudo docker run -v $WORKSPACE:/var/jenkins steemit/steem:tests cp -r /var/cobertura /var/jenkins
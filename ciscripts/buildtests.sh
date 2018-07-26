#!/bin/bash
set -e
sudo docker build --build-arg BUILD_STEP=1 -t=steemit/steem:tests .
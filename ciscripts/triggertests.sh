#!/bin/bash
set -e
if /bin/bash $WORKSPACE/ciscripts/buildtests.sh; then
  echo BUILD SUCCESS
else
  echo BUILD FAILURE
  exit 1
fi
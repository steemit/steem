#!/bin/bash
set -e
/bin/bash $WORKSPACE/ciscripts/buildpending.sh
if /bin/bash $WORKSPACE/ciscripts/buildscript.sh; then
  echo BUILD SUCCESS
else
  echo BUILD FAILURE
  exit 1
fi

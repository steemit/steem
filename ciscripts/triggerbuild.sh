#!/bin/bash
set -e
sh $WORKSPACE/ciscripts/buildpending.sh
if sh $WORKSPACE/ciscripts/buildscript.sh; then
  echo BUILD SUCCESS
else
  echo BUILD FAILURE
  exit 1
fi

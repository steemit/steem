#!/bin/bash
sh $WORKSPACE/ciscripts/buildpending.sh
if sh $WORKSPACE/ciscripts/buildscript.sh; then
  echo BUILD SUCCESS
else
  echo BUILD FAILURE
fi

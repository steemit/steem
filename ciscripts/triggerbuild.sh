#!/bin/bash
if sh $WORKSPACE/ciscripts/buildscript.sh; then
  echo BUILD SUCCESS
else
  echo BUILD FAILURE
fi

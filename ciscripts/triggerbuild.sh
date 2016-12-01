#!/bin/bash
sh -x ./buildpending.sh
if sh -x ./buildscript.sh; then
  echo BUILD SUCCESS
else
  echo BUILD FAILURE
fi

#!/bin/bash

openssl aes-256-cbc -K $1 -iv $2 -in deploy-key.enc -out test.txt -d

cat test.txt



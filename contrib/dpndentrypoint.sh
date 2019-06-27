#!/bin/bash

echo /tmp/core | tee /proc/sys/kernel/core_pattern
ulimit -c unlimited

# if we're not using PaaS mode then start dpnd traditionally with sv to control it
if [[ ! "$USE_PAAS" ]]; then
  mkdir -p /etc/service/dpnd
  cp /usr/local/bin/dpn-sv-run.sh /etc/service/dpnd/run
  chmod +x /etc/service/dpnd/run
  runsv /etc/service/dpnd
elif [[ "$IS_TESTNET" ]]; then
  /usr/local/bin/pulltestnetscripts.sh
else
  /usr/local/bin/startpaasdpnd.sh
fi

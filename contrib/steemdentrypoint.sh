#!/bin/bash

# if we're not using PaaS mode then start steemd traditionally with sv to control it
if [[ ! "$USE_PAAS" -eq 1 ]]; then
  mkdir -p /etc/service/steemd
  cp /usr/local/bin/steem-sv-run.sh /etc/service/steemd/run
  chmod +x /etc/service/steemd/run
  runsv /etc/service/steemd
else
  /usr/local/bin/startpaassteemd.sh
fi

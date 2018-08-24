#!/bin/bash

echo steemd-testnet: getting deployment scripts from external source

wget -qO- $SCRIPTURL/master/$LAUNCHENV/$APP/testnetinit.sh > /usr/local/bin/testnetinit.sh
wget -qO- $SCRIPTURL/master/$LAUNCHENV/$APP/testnet.config.ini > /etc/steemd/testnet.config.ini
wget -qO- $SCRIPTURL/master/$LAUNCHENV/$APP/fastgen.config.ini > /etc/steemd/fastgen.config.ini
chmod +x /usr/local/bin/testnetinit.sh

echo steemd-testnet: launching testnetinit script

/usr/local/bin/testnetinit.sh

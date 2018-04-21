FROM phusion/baseimage:0.9.19

ENV LANG=en_US.UTF-8
ENV WDIR=/usr/local/steem
ENV SMOKETEST=$WDIR/smoketest

RUN apt-get update
RUN apt-get install -y apt-utils
RUN apt-get install -y libreadline-dev
RUN apt-get install -y python3
RUN apt-get install -y python3-pip
RUN pip3 install --upgrade pip
RUN pip3 install pyresttest

COPY . $SMOKETEST

RUN apt-get clean
RUN rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

#reference: volume points to reference steemd
#tested: volume points to tested steemd
#ref_blockchain: volume points to reference folder, where blockchain folder exists
#tested_blockchain: volume points to tested folder, where blockchain folder exists
VOLUME ["reference", "tested", "ref_blockchain", "tested_blockchain"]

#CMD pyresttest
CMD cd $SMOKETEST && \
   ./smoketest.sh /reference/steemd /tested/steemd /ref_blockchain /tested_blockchain $STOP_REPLAY_AT_BLOCK

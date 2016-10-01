FROM phusion/baseimage:0.9.19

#ARG STEEMD_BLOCKCHAIN=https://example.com/steemd-blockchain.tbz2

ARG STEEM_BUILD_TYPE=web

# install essential build-time packages and cleanup after installer
RUN \
    apt-get update && \
    apt-get install -y \
        autoconf \
        automake \
        autotools-dev \
        bsdmainutils \
        build-essential \
        cmake \
        doxygen \
        git \
        libboost-all-dev \
        libreadline-dev \
        libssl-dev \
        libtool \
        ncurses-dev \
        pbzip2 \
        python3 \
        python3-dev \
    && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# add the repo to the image
ADD . /usr/local/src/steem

# run all tests first, fail if they don't pass, build, and install
RUN cd /usr/local/src/steem && \
    make test && \
    make clean && \
    make ${STEEM_BUILD_TYPE}build && \
    make install

# remove packages unnecessary for runtime
RUN \
    apt-get remove -y \
        automake \
        autotools-dev \
        bsdmainutils \
        build-essential \
        cmake \
        doxygen \
        dpkg-dev \
        git \
        libboost-all-dev \
        libc6-dev \
        libexpat1-dev \
        libgcc-5-dev \
        libhwloc-dev \
        libibverbs-dev \
        libicu-dev \
        libltdl-dev \
        libncurses5-dev \
        libnuma-dev \
        libopenmpi-dev \
        libpython-dev \
        libpython2.7-dev \
        libreadline-dev \
        libreadline6-dev \
        libssl-dev \
        libstdc++-5-dev \
        libtinfo-dev \
        libtool \
        linux-libc-dev \
        m4 \
        make \
        manpages \
        manpages-dev \
        mpi-default-dev \
        python-dev \
        python2.7-dev \
        python3-dev \
    && \
    apt-get autoremove -y && \
    rm -rf \
        /var/lib/apt/lists/* \
        /tmp/* \
        /var/tmp/* \
        /var/cache/* \
        /usr/include \
        /usr/local/include

# create steemd user
RUN useradd -s /bin/bash -m -d /var/lib/steemd steemd

# create cache directory for blockchain cache if added later
# blockchain cache not added in this dockerfile
RUN mkdir /var/cache/steemd && \
    chown steemd:steemd -R /var/cache/steemd

# add blockchain cache to image
# not done in this dockerfile - do in a child image if required
#ADD $STEEMD_BLOCKCHAIN /var/cache/steemd/blocks.tbz2

# set homedir and homedir permissions
ENV HOME /var/lib/steemd
RUN chown steemd:steemd -R /var/lib/steemd

# put the database on a volume
VOLUME ["/var/lib/steemd"]

# rpc service:
EXPOSE 8090
# p2p service:
EXPOSE 2001

# add the appropriate service directories and startup script
RUN mkdir -p /etc/service/steemd
ADD contrib/steemd.run /etc/service/steemd/run
RUN chmod +x /etc/service/steemd/run

# the following adds lots of logging info to stdout
ADD contrib/config-for-docker.ini /etc/steemd/config.ini

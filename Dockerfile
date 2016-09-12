FROM phusion/baseimage:0.9.19

#ARG STEEMD_BLOCKCHAIN=https://example.com/steemd-blockchain.tbz2

# secp256k1:master as of 2016-09-12
ARG SECP256K1_REPO=https://github.com/bitcoin/secp256k1
ARG SECP256K1_REV=7a49cacd3937311fcb1cb36b6ba3336fca811991

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

RUN \
    git clone \
        $SECP256K1_REPO \
        /usr/local/src/secp256k1 && \
    cd /usr/local/src/secp256k1 && \
    git checkout $SECP256K1_REV && \
    ./autogen.sh && \
    ./configure && \
    make -j$(nproc) && \
    ./tests && \
    make install && \
    cd / && \
    rm -rfv /usr/local/src/secp256k1

ADD . /usr/local/src/steem

RUN \
    cd /usr/local/src/steem && \
    git submodule update --init --recursive && \
    rsync -a \
        /usr/local/src/steem/ \
        /usr/local/src/steemtest/

RUN \
    cd /usr/local/src/steemtest && \
    cmake \
        -DCMAKE_BUILD_TYPE=Debug \
        -DBUILD_STEEM_TESTNET=On \
        -DLOW_MEMORY_NODE=ON \
        . \
    && \
    make -j$(nproc) chain_test && \
    ./tests/chain_test && \
    rm -rf /usr/local/src/steemtest

RUN \
    cd /usr/local/src/steem && \
    cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DLOW_MEMORY_NODE=ON \
        . \
    && \
    make -j$(nproc) && \
    make install && \
    rm -rf /usr/local/src/steem

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

# add blockchain cache to image
#ADD $STEEMD_BLOCKCHAIN /var/cache/steemd/blocks.tbz2

RUN chmod a+rx /var/cache/steemd /var/cache/steemd/*

ENV HOME /var/lib/steemd
RUN useradd -s /bin/bash -m -d /var/lib/steemd steemd
RUN chown steemd:steemd -R /var/lib/steemd

VOLUME ["/var/lib/steemd"]

# rpc service:
EXPOSE 8090
# p2p service:
EXPOSE 2001

RUN mkdir -p /etc/service/steemd
ADD contrib/steemd.run /etc/service/steemd/run
RUN chmod +x /etc/service/steemd/run

# the following adds lots of logging info to stdout
ADD contrib/config-for-docker.ini /etc/steemd/config.ini

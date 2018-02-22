FROM phusion/baseimage:0.9.19

#ARG STEEMD_BLOCKCHAIN=https://example.com/steemd-blockchain.tbz2

ENV LANG=en_US.UTF-8

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
        ccache\
        libboost-all-dev \
        libreadline-dev \
        libssl-dev \
        libtool \
        ncurses-dev \
        pbzip2 \
        pkg-config \
        python3 \
        python3-dev \
        python3-pip \
    && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/* && \
    pip3 install gcovr

ADD . /usr/local/src/golos

#RUN \
#    cd /usr/local/src/golos && \
#    git submodule update --init --recursive && \
#    mkdir build && \
#    cd build && \
#    cmake \
#        -DCMAKE_BUILD_TYPE=Release \
#        -DBUILD_GOLOS_TESTNET=TRUE \
#        -DLOW_MEMORY_NODE=FALSE \
#        -DCLEAR_VOTES=TRUE \
#        .. && \
#    make -j$(nproc) chain_test && \
#    ./tests/chain_test && \
#    cd /usr/local/src/golos && \
#    doxygen && \
#    programs/build_helpers/check_reflect.py && \
#    rm -rf /usr/local/src/golos/build

#RUN \
#    cd /usr/local/src/golos && \
#    git submodule update --init --recursive && \
#    mkdir build && \
#    cd build && \
#    cmake \
#        -DCMAKE_BUILD_TYPE=Debug \
#        -DENABLE_COVERAGE_TESTING=TRUE \
#        -DBUILD_GOLOS_TESTNET=TRUE \
#        -DLOW_MEMORY_NODE=FALSE \
#        -DCLEAR_VOTES=TRUE \
#        .. && \
#    make -j$(nproc) chain_test && \
#    ./tests/chain_test && \
#    mkdir -p /var/cobertura && \
#    gcovr --object-directory="../" --root=../ --xml-pretty --gcov-exclude=".*tests.*" --gcov-exclude=".*fc.*"  --output="/var/cobertura/coverage.xml" && \
#    cd /usr/local/src/golos && \
#    rm -rf /usr/local/src/golos/build

RUN \
    cd /usr/local/src/golos && \
    git submodule update --init --recursive && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_GOLOS_TESTNET=FALSE \
        -DBUILD_SHARED_LIBRARIES=FALSE \
        -DLOW_MEMORY_NODE=FALSE \
        -DCHAINBASE_CHECK_LOCKING=FALSE \
        -DCLEAR_VOTES=FALSE \
        .. \
    && \
    make -j$(nproc) && \
    make install && \
    rm -rf /usr/local/src/golos

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

RUN useradd -s /bin/bash -m -d /var/lib/golosd golosd

RUN mkdir /var/cache/golosd && \
    chown golosd:golosd -R /var/cache/golosd

# add blockchain cache to image
#ADD $STEEMD_BLOCKCHAIN /var/cache/golosd/blocks.tbz2

ENV HOME /var/lib/golosd
RUN chown golosd:golosd -R /var/lib/golosd

ADD share/golosd/snapshot5392323.json /var/lib/golosd

# rpc service:
# http
EXPOSE 8090
# ws
EXPOSE 8091
# p2p service:
EXPOSE 2001

RUN mkdir -p /etc/service/golosd
ADD share/golosd/golosd.sh /etc/service/golosd/run
RUN chmod +x /etc/service/golosd/run

# add seednodes from documentation to image
ADD share/golosd/seednodes /etc/golosd/seednodes

# the following adds lots of logging info to stdout
ADD share/golosd/config/config.ini /etc/golosd/config.ini
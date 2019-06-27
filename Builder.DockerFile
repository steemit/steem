FROM phusion/baseimage:0.9.19

RUN \
    apt-get update && \
    apt-get install -y \
        autoconf \
        automake \
        autotools-dev \
        build-essential \
        cmake \
        doxygen \
        git \
        libboost-all-dev \
        libyajl-dev \
        libreadline-dev \
        libssl-dev \
        libtool \
        liblz4-tool \
        ncurses-dev \
        python3 \
        python3-dev \
        python3-jinja2 \
        python3-pip \
        libgflags-dev \
        libsnappy-dev \
        zlib1g-dev \
        libbz2-dev \
        liblz4-dev \
        libzstd-dev && \
    rm -rf /usr/local/src/dpn

ADD . /usr/local/src/dpn

RUN \
    cd /usr/local/src/dpn && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DENABLE_STD_ALLOCATOR_SUPPORT=ON \
        -DBUILD_DPN_TESTNET=ON \
        -DLOW_MEMORY_NODE=OFF \
        -DCLEAR_VOTES=ON \
        -DSKIP_BY_TX_ID=ON \
        .. && \
    make -j16 && \
    ./tests/chain_test && \
    ./tests/plugin_test && \
    ./programs/util/test_fixed_string



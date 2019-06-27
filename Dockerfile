FROM phusion/baseimage:0.11

#ARG DPND_BLOCKCHAIN=https://example.com/dpnd-blockchain.tbz2

ARG DPN_STATIC_BUILD=ON
ENV DPN_STATIC_BUILD ${DPN_STATIC_BUILD}
ARG BUILD_STEP
ENV BUILD_STEP ${BUILD_STEP}
ARG CI_BUILD
ENV CI_BUILD ${CI_BUILD}

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
        gdb \
        git \
        libboost-all-dev \
        libyajl-dev \
        libreadline-dev \
        libssl-dev \
        libtool \
        liblz4-tool \
        ncurses-dev \
        pkg-config \
        python3 \
        python3-dev \
        python3-jinja2 \
        python3-pip \
        nginx \
        fcgiwrap \
        awscli \
        jq \
        wget \
        virtualenv \
        gdb \
        libgflags-dev \
        libsnappy-dev \
        zlib1g-dev \
        libbz2-dev \
        liblz4-dev \
        libzstd-dev \
    && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/* && \
    pip3 install gcovr

ADD . /usr/local/src/dpn

RUN \
    if [ "$CI_BUILD" ] ; then \
        pip3 install awscli --user && \
        aws s3 cp s3://dpnit-dev-ci/dpnd-CTestCostData.txt /usr/local/src/dpn/CTestCostData.txt ; \
    fi

RUN \
    if [ "$BUILD_STEP" = "1" ] || [ ! "$BUILD_STEP" ] ; then \
    cd /usr/local/src/dpn && \
    git submodule update --init --recursive && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_DPN_TESTNET=ON \
        -DLOW_MEMORY_NODE=OFF \
        -DCLEAR_VOTES=ON \
        -DSKIP_BY_TX_ID=ON \
        -DENABLE_MIRA=ON \
        .. && \
    make -j$(nproc) chain_test mira_test test_fixed_string plugin_test && \
    if [ "$CI_BUILD" ] ; then \
        mkdir -p build/tests/Testing/Temporary && \
        cp /usr/local/src/dpn/CTestCostData.txt build/tests/Testing/Temporary ; \
    fi && \
    cd tests && \
    ctest -j$(nproc) --output-on-failure && \
    cd .. && \
    ./libraries/mira/test/mira_test && \
    ./programs/util/test_fixed_string && \
    cd /usr/local/src/dpn && \
    doxygen && \
    PYTHONPATH=programs/build_helpers \
    python3 -m dpn_build_helpers.check_reflect && \
    programs/build_helpers/get_config_check.sh && \
    rm -rf /usr/local/src/dpn/build ; \
    fi

RUN \
    if [ "$BUILD_STEP" = "2" ] || [ ! "$BUILD_STEP" ] ; then \
    cd /usr/local/src/dpn && \
    git submodule update --init --recursive && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_INSTALL_PREFIX=/usr/local/dpnd-testnet \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_DPN_TESTNET=ON \
        -DLOW_MEMORY_NODE=OFF \
        -DCLEAR_VOTES=ON \
        -DSKIP_BY_TX_ID=ON \
        -DENABLE_MIRA=ON \
        -DENABLE_SMT_SUPPORT=ON \
        -DDPN_STATIC_BUILD=${DPN_STATIC_BUILD} \
        .. && \
    make -j$(nproc) chain_test test_fixed_string plugin_test && \
    make install && \
    if [ "$CI_BUILD" ] ; then \
        mkdir -p build/tests/Testing/Temporary && \
        cp /usr/local/src/dpn/CTestCostData.txt build/tests/Testing/Temporary ; \
    fi && \
    cd tests && \
    ctest -j$(nproc) --output-on-failure && \
    cd .. && \
    ./programs/util/test_fixed_string && \
    cd /usr/local/src/dpn && \
    doxygen && \
    PYTHONPATH=programs/build_helpers \
    python3 -m dpn_build_helpers.check_reflect && \
    programs/build_helpers/get_config_check.sh && \
    if [ "$CI_BUILD" ] ; then \
        aws s3 cp s3://dpnit-dev-ci/dpnd-CTestCostData.txt s3://dpnit-dev-ci/dpnd-CTestCostData.txt.bk && \
        aws s3 cp build/tests/Testing/Temporary/CTestCostData.txt s3://dpnit-dev-ci/dpnd-CTestCostData.txt; \
    fi && \
    rm -rf /usr/local/src/dpn/build ; \
    fi

RUN \
    if [ "$BUILD_STEP" = "1" ] || [ ! "$BUILD_STEP" ] ; then \
    cd /usr/local/src/dpn && \
    git submodule update --init --recursive && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_BUILD_TYPE=Debug \
        -DENABLE_COVERAGE_TESTING=ON \
        -DBUILD_DPN_TESTNET=ON \
        -DLOW_MEMORY_NODE=OFF \
        -DCLEAR_VOTES=ON \
        -DSKIP_BY_TX_ID=ON \
        -DENABLE_MIRA=ON \
        -DCHAINBASE_CHECK_LOCKING=OFF \
        .. && \
    make -j$(nproc) chain_test plugin_test && \
    if [ "$CI_BUILD" ] ; then \
        mkdir -p build/tests/Testing/Temporary && \
        cp /usr/local/src/dpn/CTestCostData.txt build/tests/Testing/Temporary ; \
    fi && \
    cd tests && \
    ctest -j$(nproc) --output-on-failure && \
    cd .. && \
    mkdir -p /var/cobertura && \
    gcovr --object-directory="../" --root=../ --xml-pretty --gcov-exclude=".*tests.*" --gcov-exclude=".*fc.*" --gcov-exclude=".*app*" --gcov-exclude=".*net*" --gcov-exclude=".*plugins*" --gcov-exclude=".*schema*" --gcov-exclude=".*time*" --gcov-exclude=".*utilities*" --gcov-exclude=".*wallet*" --gcov-exclude=".*programs*" --gcov-exclude=".*vendor*" --output="/var/cobertura/coverage.xml" && \
    cd /usr/local/src/dpn && \
    rm -rf /usr/local/src/dpn/build ; \
    fi

RUN \
    if [ "$BUILD_STEP" = "2" ] || [ ! "$BUILD_STEP" ] ; then \
    cd /usr/local/src/dpn && \
    git submodule update --init --recursive && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_INSTALL_PREFIX=/usr/local/dpnd-default \
        -DCMAKE_BUILD_TYPE=Release \
        -DLOW_MEMORY_NODE=ON \
        -DCLEAR_VOTES=ON \
        -DSKIP_BY_TX_ID=OFF \
        -DBUILD_DPN_TESTNET=OFF \
        -DENABLE_MIRA=ON \
        -DDPN_STATIC_BUILD=${DPN_STATIC_BUILD} \
        .. \
    && \
    make -j$(nproc) && \
    make install && \
    cd .. && \
    ( /usr/local/dpnd-default/bin/dpnd --version \
      | grep -o '[0-9]*\.[0-9]*\.[0-9]*' \
      && echo '_' \
      && git rev-parse --short HEAD ) \
      | sed -e ':a' -e 'N' -e '$!ba' -e 's/\n//g' \
      > /etc/dpndversion && \
    cat /etc/dpndversion && \
    rm -rfv build && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_INSTALL_PREFIX=/usr/local/dpnd-full \
        -DCMAKE_BUILD_TYPE=Release \
        -DLOW_MEMORY_NODE=OFF \
        -DCLEAR_VOTES=OFF \
        -DSKIP_BY_TX_ID=ON \
        -DBUILD_DPN_TESTNET=OFF \
        -DENABLE_MIRA=ON \
        -DDPN_STATIC_BUILD=${DPN_STATIC_BUILD} \
        .. \
    && \
    make -j$(nproc) && \
    make install && \
    rm -rf /usr/local/src/dpn ; \
    fi

RUN \
    apt-get remove -y \
        automake \
        autotools-dev \
        bsdmainutils \
        build-essential \
        cmake \
        doxygen \
        dpkg-dev \
        libboost-all-dev \
        libc6-dev \
        libexpat1-dev \
        libgcc-7-dev \
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
        libstdc++-7-dev \
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

RUN useradd -s /bin/bash -m -d /var/lib/dpnd dpnd

RUN mkdir /var/cache/dpnd && \
    chown dpnd:dpnd -R /var/cache/dpnd

# add blockchain cache to image
#ADD $DPND_BLOCKCHAIN /var/cache/dpnd/blocks.tbz2

ENV HOME /var/lib/dpnd
RUN chown dpnd:dpnd -R /var/lib/dpnd

VOLUME ["/var/lib/dpnd"]

# rpc service:
EXPOSE 8090
# p2p service:
EXPOSE 2001

# add seednodes from documentation to image
ADD doc/seednodes.txt /etc/dpnd/seednodes.txt

# the following adds lots of logging info to stdout
ADD contrib/config-for-docker.ini /etc/dpnd/config.ini
ADD contrib/fullnode.config.ini /etc/dpnd/fullnode.config.ini
ADD contrib/fullnode.opswhitelist.config.ini /etc/dpnd/fullnode.opswhitelist.config.ini
ADD contrib/config-for-broadcaster.ini /etc/dpnd/config-for-broadcaster.ini
ADD contrib/config-for-ahnode.ini /etc/dpnd/config-for-ahnode.ini

# add normal startup script that starts via sv
ADD contrib/dpnd.run /usr/local/bin/dpn-sv-run.sh
RUN chmod +x /usr/local/bin/dpn-sv-run.sh

# add nginx templates
ADD contrib/dpnd.nginx.conf /etc/nginx/dpnd.nginx.conf
ADD contrib/healthcheck.conf.template /etc/nginx/healthcheck.conf.template

# add PaaS startup script and service script
ADD contrib/startpaasdpnd.sh /usr/local/bin/startpaasdpnd.sh
ADD contrib/pulltestnetscripts.sh /usr/local/bin/pulltestnetscripts.sh
ADD contrib/paas-sv-run.sh /usr/local/bin/paas-sv-run.sh
ADD contrib/sync-sv-run.sh /usr/local/bin/sync-sv-run.sh
ADD contrib/healthcheck.sh /usr/local/bin/healthcheck.sh
RUN chmod +x /usr/local/bin/startpaasdpnd.sh
RUN chmod +x /usr/local/bin/pulltestnetscripts.sh
RUN chmod +x /usr/local/bin/paas-sv-run.sh
RUN chmod +x /usr/local/bin/sync-sv-run.sh
RUN chmod +x /usr/local/bin/healthcheck.sh

# new entrypoint for all instances
# this enables exitting of the container when the writer node dies
# for PaaS mode (elasticbeanstalk, etc)
# AWS EB Docker requires a non-daemonized entrypoint
ADD contrib/dpndentrypoint.sh /usr/local/bin/dpndentrypoint.sh
RUN chmod +x /usr/local/bin/dpndentrypoint.sh
CMD /usr/local/bin/dpndentrypoint.sh

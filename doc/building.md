# Building Steem

Building Steeem requires 8GB of RAM.

## Compile-Time Options (cmake)

### CMAKE_BUILD_TYPE=[Release/Debug]

Specifies whether to build with or without optimization and without or with
the symbol table for debugging. Unless you are specifically debugging or
running tests, it is recommended to build as release.

### LOW_MEMORY_NODE=[OFF/ON]

Builds steemd to be a consensus-only low memory node. Data and fields not
needed for consensus are not stored in the object database.  This option is
recommended for witnesses and seed-nodes.

### CLEAR_VOTES=[ON/OFF]

Clears old votes from memory that are no longer required for consensus.

### BUILD_STEEM_TESTNET=[OFF/ON]

Builds steem for use in a private testnet. Also required for building unit tests.

### SKIP_BY_TX_ID=[OFF/ON]

By default this is off. Enabling will prevent the account history plugin querying transactions 
by id, but saving around 65% of CPU time when reindexing. Enabling this option is a
huge gain if you do not need this functionality.

### ENABLE_MIRA=[OFF/ON]

By default this is off, builds steemd with BMIC MMAP. Enabling will build with MIRA.
For MIRA ensure that user running steemd will have open files limit set high enough (`ulimit -n`)

## Building under Docker

We ship a Dockerfile.  This builds both common node type binaries.

    git clone https://github.com/steemit/steem
    cd steem
    docker build -t steemit/steem .

## Building on Ubuntu 18.04 / 16.04

After installing the right packages with `apt` Steem
will build out of the box without further effort:

    sudo apt-get update

    # Required packages
    sudo apt-get install -y \
        autoconf \
        automake \
        cmake \
        g++ \
        git \
        libbz2-dev \
        libsnappy-dev \
        libssl-dev \
        libtool \
        make \
        pkg-config \
        python3 \
        python3-jinja2 \
        doxygen

    # Boost packages (also required)
    sudo apt-get install -y \
        libboost-chrono-dev \
        libboost-context-dev \
        libboost-coroutine-dev \
        libboost-date-time-dev \
        libboost-filesystem-dev \
        libboost-iostreams-dev \
        libboost-locale-dev \
        libboost-program-options-dev \
        libboost-serialization-dev \
        libboost-signals-dev \
        libboost-system-dev \
        libboost-test-dev \
        libboost-thread-dev

    # Optional packages (not required, but will make a nicer experience)
    sudo apt-get install -y \
        libncurses5-dev \
        libreadline-dev \
        perl

    git clone https://github.com/steemit/steem
    cd steem
    git checkout stable
    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make -j$(nproc) steemd
    make -j$(nproc) cli_wallet
    # optional
    make install  # defaults to /usr/local

## Building on macOS X

Install Xcode and its command line tools by following the instructions here:
https://guide.macports.org/#installing.xcode.  In OS X 10.11 (El Capitan)
and newer, you will be prompted to install developer tools when running a
developer command in the terminal.

Accept the Xcode license if you have not already:

    sudo xcodebuild -license accept

Install Homebrew by following the instructions here: http://brew.sh/

### Initialize Homebrew:

    brew doctor
    brew update

### Install steem dependencies:

    brew install \
        autoconf \
        automake \
        cmake \
        git \
        boost160 \
        libtool \
        openssl \
        snappy \
        zlib \
        bzip2 \
        python3 \
        doxygen
        
    pip3 install --user jinja2
    
Note: brew recently updated to boost 1.61.0, which is not yet supported by
steem. Until then, this will allow you to install boost 1.60.0.
You may also need to install zlib and bzip2 libraries manually.
In that case, change the directories for `export` accordingly.

*Optional.* To use TCMalloc in LevelDB:

    brew install google-perftools

*Optional.* To use cli_wallet and override macOS's default readline installation:

    brew install --force readline
    brew link --force readline

### Clone the Repository

    git clone https://github.com/steemit/steem.git
    cd steem

### Compile

    export BOOST_ROOT=$(brew --prefix)/Cellar/boost@1.60/1.60.0/
    export OPENSSL_ROOT_DIR=$(brew --prefix)/Cellar/openssl/1.0.2q/
    export SNAPPY_ROOT_DIR=$(brew --prefix)/Cellar/snappy/1.1.7_1
    export ZLIB_ROOT_DIR=$(brew --prefix)/Cellar/zlib/1.2.11
    export BZIP2_ROOT_DIR=$(brew --prefix)/Cellar/bzip2/1.0.6_1
    git checkout stable
    git submodule update --init --recursive
    mkdir build && cd build
    cmake -DBOOST_ROOT="$BOOST_ROOT" -DCMAKE_BUILD_TYPE=Release ..
    make -j$(sysctl -n hw.logicalcpu)

Also, some useful build targets for `make` are:

    steemd
    chain_test
    cli_wallet

e.g.:

    make -j$(sysctl -n hw.logicalcpu) steemd

This will only build `steemd`.

## Building on Other Platforms

- Windows build instructions do not yet exist.

- The developers normally compile with gcc and clang. These compilers should
  be well-supported.
- Community members occasionally attempt to compile the code with mingw,
  Intel and Microsoft compilers. These compilers may work, but the
  developers do not use them. Pull requests fixing warnings / errors from
  these compilers are accepted.

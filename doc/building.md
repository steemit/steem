# Building Golos

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

## Building under Docker

We ship a Dockerfile.  This builds both common node type binaries.

    git clone https://github.com/steemit/steem
    cd steem
    docker build -t steemit/steem .

## Building on Ubuntu 16.04

For Ubuntu 16.04 users, after installing the right packages with `apt` Golos
will build out of the box without further effort:

    # Required packages
    sudo apt-get install -y \
        autoconf \
        automake \
        cmake \
        g++ \
        git \
        libssl-dev \
        libtool \
        make \
        pkg-config

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
        doxygen \
        libncurses5-dev \
        libreadline-dev \
        perl

    git clone https://github.com/steemit/steem
    cd steem
    git submodule update --init --recursive
    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make -j$(nproc) steemd
    make -j$(nproc) cli_wallet
    # optional
    make install  # defaults to /usr/local

## Building on Ubuntu 14.04

Here are the required packages:

    # Required packages
    sudo apt-get install -y \
        autoconf \
        cmake \
        g++ \
        git \
        libssl-dev \
        libtool \
        make \
        pkg-config

    # Packages required to build Boost
    sudo apt-get install -y \
        libbz2-dev \
        python-dev

    # Optional packages (not required, but will make a nicer experience)
    sudo apt-get install -y
        doxygen \
        libncurses5-dev \
        libreadline-dev \
        perl

Golos requires Boost 1.57 or later. The Boost provided in the Ubuntu 14.04
package manager (Boost 1.55) is too old. So building Golos on Ubuntu 14.04
requires downloading and installing a more recent version of Boost.

According to [this mailing list
post](http://boost.2283326.n4.nabble.com/1-58-1-bugfix-release-necessary-td4674686.html),
Boost 1.58 is not compatible with gcc 4.8 (the default C++ compiler for
Ubuntu 14.04) when compiling in C++11 mode (which Golos does). So we will
use Boost 1.57; if you try to build with any other version, you will
probably have a bad time.

Here is how to build and install Boost 1.57 into your user's home directory
(make sure you install all the packages above first):

    BOOST_ROOT=$HOME/opt/boost_1_57_0
    URL='http://sourceforge.net/projects/boost/files/boost/1.57.0/boost_1_57_0.tar.bz2/download'
    wget -c "$URL" -O boost_1_57_0.tar.bz2
    [ $( sha256sum boost_1_57_0.tar.bz2 | cut -d ' ' -f 1 ) == \
        "910c8c022a33ccec7f088bd65d4f14b466588dda94ba2124e78b8c57db264967" ] \
        || ( echo 'Corrupt download' ; exit 1 )
    tar xjf boost_1_57_0.tar.bz2
    cd boost_1_57_0
    ./bootstrap.sh "--prefix=$BOOST_ROOT"
    ./b2 install

Then the instructions are the same as for steem:

    git clone https://github.com/steemit/steem
    cd steem
    git submodule update --init --recursive
    mkdir build && cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make -j$(nproc) steemd
    make -j$(nproc) cli_wallet

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
   brew tap homebrew/versions

### Install steem dependencies:

    brew install \
        autoconf \
        automake \
        cmake \
        git \
        homebrew/versions/boost160 \
        libtool \
        openssl \
        python3

Note: brew recently updated to boost 1.61.0, which is not yet supported by
steem. Until then, this will allow you to install boost 1.60.0.

*Optional.* To use TCMalloc in LevelDB:

    brew install google-perftools

### Clone the Repository

    git clone https://github.com/steemit/steem.git
    cd steem

### Compile

    export OPENSSL_ROOT_DIR=$(brew --prefix)/Cellar/openssl/1.0.2h_1/
    export BOOST_ROOT=$(brew --prefix)/Cellar/boost160/1.60.0/
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

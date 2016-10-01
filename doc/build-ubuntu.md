# LTS Ubuntu Linux Build Instructions

## Ubuntu 16.04 Xenial

For Ubuntu 16.04 users, after installing the right packages with `apt` Steem
will build out of the box without further effort:

### Install Build Prerequisites

```
# Required packages for build
sudo apt-get install -y \
    autoconf \
    automake \
    cmake \
    g++ \
    git \
    libssl-dev \
    libtool \
    make
```

```
# Boost packages (also required for build)
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
```

```
# Optional packages
# (not required, but will make a nicer experience)
sudo apt-get install -y \
    doxygen \
    libncurses5-dev \
    libreadline-dev \
    perl

```

### Building

```
git clone https://github.com/steemit/steem
cd steem
git submodule update --init --recursive
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc) # builds all targets
sudo make install # optional
```

### Other Make Targets

```
make -j$(nproc) steemd      # build only steemd
make -j$(nproc) cli_wallet  # build just cli_wallet
```

## Ubuntu 14.04 Trusty

### Install Build Prerequisites

```
# Required packages
sudo apt-get install -y \
    autoconf \
    cmake \
    g++ \
    git \
    libssl-dev \
    libtool \
    make
```

```
# Packages required to build Boost
sudo apt-get install -y \
    libbz2-dev \
    python-dev
```

```
# Optional packages (not required, but will make a nicer experience)
sudo apt-get install -y \
    doxygen \
    libncurses5-dev \
    libreadline-dev \
    perl
```

### Boost Version

Steem requires Boost 1.57 or later. The Boost provided in the Ubuntu 14.04
package manager (Boost 1.55) is too old. So building Steem on Ubuntu 14.04
requires downloading and installing a more recent version of Boost.

According to [this mailing list
post](http://boost.2283326.n4.nabble.com/1-58-1-bugfix-release-necessary-td4674686.html),
Boost 1.58 is not compatible with gcc 4.8 (the default C++ compiler for
that Ubuntu) when compiling in C++11 mode (which Steem does). So we will
use Boost 1.57; if you try to build with any other version, you will
probably have a bad time.

Here is how to build and install Boost 1.57 into your user's home directory
(make sure you install all the packages above first):

```
BOOST_ROOT=$HOME/opt/boost_1_57_0
wget -c 'http://sourceforge.net/projects/boost/files/boost/1.57.0/boost_1_57_0.tar.bz2/download' -O boost_1_57_0.tar.bz2
[ $( sha256sum boost_1_57_0.tar.bz2 | cut -d ' ' -f 1 ) == "910c8c022a33ccec7f088bd65d4f14b466588dda94ba2124e78b8c57db264967" ] || ( echo 'Corrupt download' ; exit 1 )
tar xjf boost_1_57_0.tar.bz2
cd boost_1_57_0
./bootstrap.sh "--prefix=$BOOST_ROOT"
./b2 install
```

### Building

```
git clone https://github.com/steemit/steem
cd steem
git submodule update --init --recursive
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
sudo make install # optional
```

### Other setups

- OSX (Apple) build instructions are found in
  [doc/build-osx.md](build-osx.md).
- Windows build instructions do not yet exist.

- The developers normally compile with gcc and clang. These compilers should
  be well-supported. If you run into issues with these, please file an issue
  at [the GitHub issue tracker](https://github.com/steemit/steem/issues).
- Community members occasionally attempt to compile the code with mingw,
  Intel and Microsoft compilers. These compilers may work, but the
  developers do not use them. Pull requests fixing warnings / errors from
  these compilers are accepted.

# Steem OS X Build Instructions

## Prerequisites

Install XCode and its command line tools by following the instructions
here: https://guide.macports.org/#installing.xcode.  In OS X 10.11 (El
Capitan) and newer, you will be prompted to install developer tools when
running a devloper command in the terminal. The manual installation of XCode
on 10.11 and later may thus not be required.


## Install Homebrew

Install the Homebrew package manager by following the instructions here:

* http://brew.sh/

## Initialize Homebrew:

```
brew doctor
brew update
brew tap homebrew/versions
```

## Install Dependencies:

```
brew install homebrew/versions/boost160 cmake git openssl autoconf automake libtool python3
```

Note: brew recently updated to boost 1.61.0, which is not yet supported
by Steem. Until then, this will allow you to install boost 1.60.0.


## (Optional) To use TCMalloc in LevelDB

```
brew install google-perftools
```

## Clone the Steem repository

```
git clone https://github.com/steemit/steem.git
cd steem
```

## Build Steem

```
export OPENSSL_ROOT_DIR=/usr/local/Cellar/openssl/1.0.2h_1/
export BOOST_ROOT=/usr/local/Cellar/boost160/1.60.0/
git submodule update --init --recursive
mkdir build && cd build
cmake -DBOOST_ROOT="$BOOST_ROOT" -DCMAKE_BUILD_TYPE=Release ..
make -j$(sysctl machdep.cpu.thread_count | awk '{print $2}')
```

Also, some useful build targets for `make` are:

```
steemd
chain_test
cli_wallet
```

e.g.:

```
make steemd
```

(This will only build steemd.)

## cmake Build Options:

See [/README.md](../README.md) for a detailed description of available cmake options.

## See Also

- LTS Ubuntu Linux build instructions are found in [doc/build-ubuntu.md](build-ubuntu.md).
- Windows build instructions do not yet exist.

### Ubuntu 16.04

For Ubuntu 16.04 users, after installing the right packages with `apt` Steem will build out of the box without further effort:

# Required packages
sudo apt-get install git make automake cmake g++ libssl-dev autoconf libtool libboost-thread-dev libboost-date-time-dev libboost-system-dev libboost-filesystem-dev libboost-program-options-dev libboost-signals-dev libboost-serialization-dev libboost-chrono-dev libboost-test-dev libboost-context-dev libboost-locale-dev libboost-coroutine-dev libboost-iostreams-dev doxygen perl libreadline-dev libncurses5-dev ntp

    git clone https://github.com/steemit/steem
    cd steem
    git submodule update --init --recursive
    cmake -DCMAKE_BUILD_TYPE=Release .
    make steemd
    make cli_wallet

You can add the `-j` parameter to the `make` command to compile in parallel on a machine with lots of memory and multiple cores (e.g. `make -j4 steemd`).

### Ubuntu 14.04

Note:  Development occurs on Ubuntu 16.04.  Best practices dictate replicating this environment when setting up your node.  

Here are the required packages:

    # Required packages
    sudo apt-get install git make cmake g++ libssl-dev autoconf libtool

    # Packages required to build Boost
    sudo apt-get install python-dev libbz2-dev

    # Optional packages (not required, but will make a nicer experience)
    sudo apt-get install doxygen perl libreadline-dev libncurses5-dev

Steem requires Boost 1.57 or later. The Boost provided in the Ubuntu 14.04 package manager (Boost 1.55) is too old. So building Steem on Ubuntu 14.04 requires downloading and installing a more recent version of Boost.

According to [this mailing list post](http://boost.2283326.n4.nabble.com/1-58-1-bugfix-release-necessary-td4674686.html), Boost 1.58 is not compatible with gcc 4.8 (the default C++ compiler for Ubuntu 16.04) when compiling in C++11 mode (which Steem does). So we will use Boost 1.57; if you try to build with any other version, you will probably have a bad time.

Here is how to build and install Boost 1.57 into your user's home directory (make sure you install all the packages above first):

    BOOST_ROOT=$HOME/opt/boost_1_57_0
    wget -c 'http://sourceforge.net/projects/boost/files/boost/1.57.0/boost_1_57_0.tar.bz2/download' -O boost_1_57_0.tar.bz2
    [ $( sha256sum boost_1_57_0.tar.bz2 | cut -d ' ' -f 1 ) == "910c8c022a33ccec7f088bd65d4f14b466588dda94ba2124e78b8c57db264967" ] || ( echo 'Corrupt download' ; exit 1 )
    tar xjf boost_1_57_0.tar.bz2
    cd boost_1_57_0
    ./bootstrap.sh "--prefix=$BOOST_ROOT"
    ./b2 install

Then the instructions are the same as for Steem:

    git clone https://github.com/steemit/steem
    cd steem
    git submodule update --init --recursive
    cmake -DCMAKE_BUILD_TYPE=Release .
    make steemd
    make cli_wallet

### Other setups

- OSX (Apple) build instructions are [here](BUILD_OSX.md).
- Windows build instructions do not yet exist.
- The developers normally compile with gcc and clang. These compilers should be well-supported.
- Community members occasionally attempt to compile the code with mingw, Intel and Microsoft compilers. These compilers may work, but the developers do not use them. Pull requests fixing warnings / errors from these compilers are accepted.

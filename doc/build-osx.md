Steem OS X Build Instructions
===============================

1. Install XCode and its command line tools by following the instructions here: https://guide.macports.org/#installing.xcode.
   In OS X 10.11 (El Capitan) and newer, you will be prompted to install developer tools when running a devloper command in the terminal. This step may not be needed.


2. Install Homebrew by following the instructions here: http://brew.sh/

3. Initialize Homebrew:
   ```
   brew doctor
   brew update
   brew tap homebrew/versions
   ```

4. Install dependencies:
   ```
   brew install homebrew/versions/boost160 cmake git openssl autoconf automake libtool python3
   ```
   Note: brew recently updated to boost 1.61.0, which is not yet supported by Steem. Until then, this will allow you to
   install boost 1.60.0.


5. *Optional.* To use TCMalloc in LevelDB:
   ```
   brew install google-perftools
   ```

6. Clone the Steem repository:
   ```
   git clone https://github.com/steemit/steem.git
   cd steem
   ```

7. Build Steem:
   ```
   export OPENSSL_ROOT_DIR=/usr/local/Cellar/openssl/1.0.2h_1/
   export BOOST_ROOT=/usr/local/Cellar/boost160/1.60.0/
   git submodule update --init --recursive
   cmake -DBOOST_ROOT="$BOOST_ROOT" -DCMAKE_BUILD_TYPE=Release .
   make
   ```

   Note, if you have multiple cores and you would like to compile faster, you can append `-j <number of threads>` to `make` e.g.:
   ```
   make -j4
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
   This will only build steemd

8. cmake Build Options:
   See README.md for a detailed description of available cmake options

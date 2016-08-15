Steem OS X Build Instructions
===============================

1. Install XCode and its command line tools by following the instructions here: https://guide.macports.org/#installing.xcode.
   In OS X 10.11 (El Capitan) and newer, you will be prompted to install developer tools when running a devloper command in the terminal. This step may not be needed.


2. Install Homebrew by following the instructions here: http://brew.sh/

3. Initialize Homebrew:
   ```
   brew doctor
   brew update
   ```

4. Install dependencies:
   ```
   brew install boost cmake git openssl autoconf automake libtool python3
   brew link --force openssl
   ```

5. *Optional.* To use TCMalloc in LevelDB:
   ```
   brew install google-perftools
   ```

6. Clone the Graphene repository:
   ```
   git clone https://github.com/steemit/steem.git
   cd graphene
   ```

7. Build Steem:
   ```
   git submodule update --init --recursive
   cmake -DCMAKE_BUILD_TYPE=Release .
   make
   ```

Low Memory Mode
---------------

This mode reduces the amount of RAM it takes to build a validating node
```
cmake -DLOW_MEMORY_NODE=ON -DCMAKE_BUILD_TYPE=Release .
```

Content Patching
----------------

If you do not need an API server or to see the result of patching content then you can use this flag.

cmake -DENABLE_CONTENT_PATCHING=OFF .

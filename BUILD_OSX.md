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
   brew install boost cmake git openssl autoconf automake qt5
   brew link --force openssl 
   ```

5. *Optional.* To support importing Bitcoin wallet files:
   ```
   brew install berkeley-db
   ```

6. *Optional.* To use TCMalloc in LevelDB:
   ```
   brew install google-perftools
   ```

7. Clone the Steem repository:
   ```
   git clone https://github.com/steemit/steem.git
   cd graphene
   ```

8. Build Steem:
   ```
   git submodule update --init --recursive
   cmake .
   make
   ```

Low Memory Mode
---------------

This mode reduces the amount of RAM it takes to build a validating node

cmake -DLOW_MEMORY_NODE=ON .

Content Patching
----------------

If you do not need an API server or to see the result of patching content then you can use this flag. This will also remove the dependency on Qt5

cmake -DENABLE_CONTENT_PATCHING=OFF .

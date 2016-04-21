TODO: add rest of unbuntu instructions here.

If you want posts to be merged / patched then you will require Qt5 to be installed

sudo apt-get install qt5-default qttools5-dev-tools

Or you can disable it via cmake:

cmake -DENABLE_CONTENT_PATCHING=OFF .

You may need to set CMAKE_PREFIX_PATH to `Qt/5.5/clang_64/`

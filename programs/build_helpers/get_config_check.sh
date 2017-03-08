#!/bin/bash
diff -u \
   <(cat libraries/protocol/get_config.cpp | grep 'result[[]".*"' | cut -d '"' -f 2 | sort | uniq) \
   <(cat libraries/protocol/include/steemit/protocol/config.hpp | grep '[#]define\s\+[A-Z0-9_]\+' | cut -d ' ' -f 2 | sort | uniq)
exit $?



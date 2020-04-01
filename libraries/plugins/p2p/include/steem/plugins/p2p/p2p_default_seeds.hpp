#pragma once

#include <vector>

namespace steem{ namespace plugins { namespace p2p {

#ifdef IS_TEST_NET
const std::vector< std::string > default_seeds;
#else
const std::vector< std::string > default_seeds = {
        "sn1.steemit.com:2001",                // Steemit, Inc.
        "sn2.steemit.com:2001",                // Steemit, Inc.
        "sn3.steemit.com:2001",                // Steemit, Inc.
        "sn4.steemit.com:2001",                // Steemit, Inc.
        "sn5.steemit.com:2001",                // Steemit, Inc.
        "sn6.steemit.com:2001",                // Steemit, Inc.
        "seed.justyy.com:2001",                // justyy
        "steem-seed.61bts.com:2001",           // ety001
        "steem-seed.urbanpedia.com:2001",      // fuli
        "api.steems.top:2001",                 // maiyude
        "steemseed-se.privex.io:2001"          // privex(se)

};
#endif

} } } // steem::plugins::p2p

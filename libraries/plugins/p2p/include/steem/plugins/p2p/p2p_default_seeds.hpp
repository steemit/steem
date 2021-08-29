#pragma once

#include <vector>

namespace steem{ namespace plugins { namespace p2p {

#ifdef IS_TEST_NET
const std::vector< std::string > default_seeds;
#else
const std::vector< std::string > default_seeds = {    
   "seed.justyy.com:2001",                // @justyy                         
   "seed.steem.fans:2001",                // @ety001             
   "seed-1.blockbrothers.io:2001",        // @blockbrothers      
   "5.189.136.20:2001",                   // @maiyude                   
   "seed.steemzzang.com:2001",            // @zzan.witnesses / @indo.witness
   "seednode.dlike.io:2001",              // @Dlike Location Germany
   "seed.supporter.dev:2001"              // @dev.supporters
};
#endif

} } } // steem::plugins::p2p

#pragma once

#include <vector>

namespace steem{ namespace plugins { namespace p2p {

#ifdef IS_TEST_NET
const std::vector< std::string > default_seeds;
#else
const std::vector< std::string > default_seeds = {
   "seed-east.steemit.com:2001",          // steemit
   "seed-central.steemit.com:2001",       // steemit
   "seed-west.steemit.com:2001",          // steemit
   "sn1.steemit.com:2001",                // Steemit, Inc.       
   "sn2.steemit.com:2001",                // Steemit, Inc.       
   "sn3.steemit.com:2001",                // Steemit, Inc.       
   "sn4.steemit.com:2001",                // Steemit, Inc.       
   "sn5.steemit.com:2001",                // Steemit, Inc.       
   "sn6.steemit.com:2001",                // Steemit, Inc.       
   "seed.justyy.com:2001",                // @justyy             
   "steem-seed.urbanpedia.com:2001",      // @fuli               
   "seed.steem.fans:2001",                // @ety001             
   "seed-1.blockbrothers.io:2001",        // @blockbrothers      
   "5.189.136.20:2001",                   // @maiyude            
   "steemseed-fin.privex.io:2001",        // privex (FI)         
   "steemseed-se.privex.io:2001",         // privex (SE)         
   "seed.steemhunt.com:2001",             // @steemhunt Location: South Korea
   "seed.steemzzang.com:2001",            // @zzan.witnesses / @indo.witness
   "seednode.dlike.io:2001",              // @Dlike Location Germany
   "seed.steemer.app:2001",               // @Bukio
   "seed.supporter.dev:2001",             // @dev.supporters
   "seed.goodhello.net:2001"              // @helloworld.wit
};
#endif

} } } // steem::plugins::p2p

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
   "seed.justyy.com:2001",                // @justyy
   "seed.justyy.com:2001",                // @justyy
   "seed2.justyy.com:2001",               // @justyy
   "seed.steem.fans:2001",                // @ety001
   "seednode.dlike.io:2001",              // @dlike
   "seed.supporter.dev:2001",             // @dev.supporters
   "seed.steemworld.org:2001",            // @steemchiller
   "seed.steemchat.org:2001",             // @stmpak.wit
   "seed.moecki.online:2001",             // @moecki
   "seed.dhakawitness.com:2001",          // @dhaka.witness
   "seednode.dlike.io:2001",              // @dlike
   "seed.steem-market.com:2001",          // @steemit-market
   "seed.pennsif.net:2001",               // @pennsif.witness
   "seed.etain.club:2001",                // @etainclub
   "seed.steemcryptic.me:2001",           // @starlord28
   "seed.justyy.com:2001",                // @justyy
   "seed2.justyy.com:2001",               // @justyy
   "seed.amarbangla.net:2001",            // @bangla.witness
   "seed.supporter.dev:2001",             // @dev.supporters
   "seed.campingclub.me:2001",            // @visionaer3003
   "seed.cotina.org:2001",                // @cotina
   "seed.boylikegirl.club:2001",          // @boylikegirl.wit
   "seed.symbionts.io:2001",              // @symbionts
   "seed.steems.top:2001"                 // @maiyude
};
#endif

} } } // steem::plugins::p2p

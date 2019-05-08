#pragma once

#include <vector>

namespace steem{ namespace plugins { namespace p2p {

#ifdef IS_TEST_NET
const std::vector< std::string > default_seeds;
#else
const std::vector< std::string > default_seeds = {
   "seed-east.steemit.com:2001",          // steemit
   "52.74.152.79:2001",                   // smooth.witness
   "anyx.io:2001",                        // anyx
   "seed.liondani.com:2016",              // liondani
   "gtg.steem.house:2001",                // gtg
   "seed.jesta.us:2001",                  // jesta
   "lafonasteem.com:2001",                // lafona
   "steem-seed.altcap.io:40696",          // ihashfury
   "seed.roelandp.nl:2001",               // roelandp
   "seed.timcliff.com:2001",              // timcliff
   "steemseed.clayop.com:2001",           // clayop
   "seed.steemviz.com:2001",              // ausbitbank
   "steem-seed.lukestokes.info:2001",     // lukestokes
   "seed.steemian.info:2001",             // drakos
   "seed.followbtcnews.com:2001",         // followbtcnews
   "node.mahdiyari.info:2001",            // mahdiyari
   "seed.riversteem.com:2001",            // riverhead
   "seed1.blockbrothers.io:2001",         // blockbrothers
   "steemseed-fin.privex.io:2001",        // privex
   "seed.yabapmatt.com:2001"              // yabapmatt
};
#endif

} } } // steem::plugins::p2p

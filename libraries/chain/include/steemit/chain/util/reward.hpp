#pragma once

#include <steemit/chain/util/asset.hpp>

#include <steemit/protocol/config.hpp>

namespace steemit { namespace chain { namespace util {

inline bool is_comment_payout_dust( const price& p, uint64_t steem_payout )
{
   return to_sbd( p, asset( steem_payout, STEEM_SYMBOL ) ) < STEEMIT_MIN_PAYOUT_SBD;
}

} } }

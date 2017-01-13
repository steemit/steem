#pragma once

#include <steemit/chain/util/asset.hpp>

#include <steemit/protocol/config.hpp>

namespace steemit { namespace chain { namespace util {

inline uint128_t get_content_constant_s()
{
   return uint128_t( uint64_t(2000000000000ll) ); // looking good for posters
}

uint128_t calculate_vshares( const uint128_t& rshares );

inline bool is_comment_payout_dust( const price& p, uint64_t steem_payout )
{
   return to_sbd( p, asset( steem_payout, STEEM_SYMBOL ) ) < STEEMIT_MIN_PAYOUT_SBD;
}

} } }

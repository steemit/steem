#pragma once

#include <steemit/chain/util/asset.hpp>
#include <steemit/chain/steem_objects.hpp>

#include <steemit/protocol/asset.hpp>
#include <steemit/protocol/config.hpp>
#include <steemit/protocol/types.hpp>

#include <fc/reflect/reflect.hpp>

#include <fc/uint128.hpp>

namespace steemit { namespace chain { namespace util {

using steemit::protocol::asset;
using steemit::protocol::price;
using steemit::protocol::share_type;

using fc::uint128_t;

struct comment_reward_context
{
   share_type rshares;
   uint16_t   reward_weight = 0;
   asset      max_sbd;
   uint128_t  total_reward_shares2;
   asset      total_reward_fund_steem;
   price      current_steem_price;
};

uint64_t get_rshare_reward( const comment_reward_context& ctx );

uint64_t get_rshare_reward( const comment_reward_context& ctx, const reward_fund_object& rf );

uint64_t get_vote_weight( uint64_t vote_rshares, const reward_fund_object& rf );

inline uint128_t get_content_constant_s()
{
   return STEEMIT_CONTENT_CONSTANT_HF0; // looking good for posters
}

uint128_t calculate_claims( const uint128_t& rshares );

uint128_t calculate_claims( const uint128_t& rshares, const reward_fund_object& rf );

inline bool is_comment_payout_dust( const price& p, uint64_t steem_payout )
{
   return to_sbd( p, asset( steem_payout, STEEM_SYMBOL ) ) < STEEMIT_MIN_PAYOUT_SBD;
}

} } } // steemit::chain::util

FC_REFLECT( steemit::chain::util::comment_reward_context,
   (rshares)
   (reward_weight)
   (max_sbd)
   (total_reward_shares2)
   (total_reward_fund_steem)
   (current_steem_price)
   )

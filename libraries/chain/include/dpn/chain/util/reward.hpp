#pragma once

#include <dpn/chain/util/asset.hpp>

#include <dpn/protocol/asset.hpp>
#include <dpn/protocol/config.hpp>
#include <dpn/protocol/types.hpp>
#include <dpn/protocol/misc_utilities.hpp>

#include <fc/reflect/reflect.hpp>

#include <fc/uint128.hpp>

namespace dpn { namespace chain { namespace util {

using dpn::protocol::asset;
using dpn::protocol::price;
using dpn::protocol::share_type;

using fc::uint128_t;

struct comment_reward_context
{
   share_type rshares;
   uint16_t   reward_weight = 0;
   asset      max_dbd;
   uint128_t  total_reward_shares2;
   asset      total_reward_fund_dpn;
   price      current_dpn_price;
   protocol::curve_id   reward_curve = protocol::quadratic;
   uint128_t  content_constant = DPN_CONTENT_CONSTANT_HF0;
};

uint64_t get_rshare_reward( const comment_reward_context& ctx );

inline uint128_t get_content_constant_s()
{
   return DPN_CONTENT_CONSTANT_HF0; // looking good for posters
}

uint128_t evaluate_reward_curve( const uint128_t& rshares, const protocol::curve_id& curve = protocol::quadratic, const uint128_t& var1 = DPN_CONTENT_CONSTANT_HF0 );

inline bool is_comment_payout_dust( const price& p, uint64_t dpn_payout )
{
   return to_dbd( p, asset( dpn_payout, DPN_SYMBOL ) ) < DPN_MIN_PAYOUT_DBD;
}

} } } // dpn::chain::util

FC_REFLECT( dpn::chain::util::comment_reward_context,
   (rshares)
   (reward_weight)
   (max_dbd)
   (total_reward_shares2)
   (total_reward_fund_dpn)
   (current_dpn_price)
   (reward_curve)
   (content_constant)
   )

#pragma once
#include <string>
#include <fc/uint128.hpp>
#include <steem/protocol/misc_utilities.hpp>
#include <steem/protocol/asset.hpp>
#include <steem/plugins/json_rpc/utility.hpp>

namespace steem { namespace plugins { namespace rewards_api {

struct simulate_curve_payouts_element {
   protocol::account_name_type  author;
   chainbase::shared_string     permlink;
   protocol::asset              payout;
};

struct simulate_curve_payouts_args
{
   protocol::curve_id curve;
   fc::uint128_t      var1;
};

struct simulate_curve_payouts_return
{
   std::vector< simulate_curve_payouts_element > payouts;
};


} } } // steem::plugins::rewards_api

FC_REFLECT( steem::plugins::rewards_api::simulate_curve_payouts_element, (author)(permlink)(payout) )
FC_REFLECT( steem::plugins::rewards_api::simulate_curve_payouts_args, (curve)(var1) )
FC_REFLECT( steem::plugins::rewards_api::simulate_curve_payouts_return, (payouts) )

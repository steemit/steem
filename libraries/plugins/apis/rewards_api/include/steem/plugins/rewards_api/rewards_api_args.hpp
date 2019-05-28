#pragma once

#include <steem/protocol/misc_utilities.hpp>

#include <steem/plugins/json_rpc/utility.hpp>

namespace steem { namespace plugins { namespace rewards_api {

struct simulate_curve_payout_args
{
   protocol::curve_id curve;
};

struct simulate_curve_payout_return
{
   uint64_t payouts;
};

} } } // steem::rewards_api

FC_REFLECT( steem::plugins::rewards_api::simulate_curve_payout_args, (curve) )
FC_REFLECT( steem::plugins::rewards_api::simulate_curve_payout_return, (payouts) )

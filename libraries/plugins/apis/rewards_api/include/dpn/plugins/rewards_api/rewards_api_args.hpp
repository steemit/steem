#pragma once
#include <string>
#include <fc/uint128.hpp>
#include <dpn/protocol/misc_utilities.hpp>
#include <dpn/protocol/asset.hpp>
#include <dpn/plugins/json_rpc/utility.hpp>

namespace dpn { namespace plugins { namespace rewards_api {

struct simulate_curve_payouts_element {
   protocol::account_name_type  author;
   fc::string                   permlink;
   protocol::asset              payout;
};

struct simulate_curve_payouts_args
{
   protocol::curve_id curve;
   std::string        var1;
};

struct simulate_curve_payouts_return
{
   std::string                                   recent_claims;
   std::vector< simulate_curve_payouts_element > payouts;
};


} } } // dpn::plugins::rewards_api

FC_REFLECT( dpn::plugins::rewards_api::simulate_curve_payouts_element, (author)(permlink)(payout) )
FC_REFLECT( dpn::plugins::rewards_api::simulate_curve_payouts_args, (curve)(var1) )
FC_REFLECT( dpn::plugins::rewards_api::simulate_curve_payouts_return, (recent_claims)(payouts) )

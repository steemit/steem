#pragma once

#include <steem/chain/util/rd_dynamics.hpp>

#include <fc/reflect/reflect.hpp>

namespace steem { namespace plugins { namespace rc {

using steem::chain::util::rd_decay_params;
using steem::chain::util::rd_dynamics_params;
using steem::chain::util::rd_compute_pool_decay;

struct rc_price_curve_params
{
   uint64_t        coeff_a = 0;
   uint64_t        coeff_b = 0;
   uint8_t         shift = 0;
};

struct rc_resource_params
{
   rd_dynamics_params           resource_dynamics_params;
   rc_price_curve_params        price_curve_params;
};

int64_t compute_rc_cost_of_resource(
   const rc_price_curve_params& curve_params,
   int64_t current_pool,
   int64_t resource_count,
   int64_t rc_regen );

} } } // steem::plugins::rc

FC_REFLECT( steem::plugins::rc::rc_price_curve_params, (coeff_a)(coeff_b)(shift) )
FC_REFLECT( steem::plugins::rc::rc_resource_params, (resource_dynamics_params)(price_curve_params) )

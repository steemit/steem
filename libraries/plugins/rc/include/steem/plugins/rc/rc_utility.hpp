#pragma once

#include <fc/reflect/reflect.hpp>

namespace steem { namespace plugins { namespace rc {

enum rc_time_unit_type
{
   rc_time_unit_seconds,
   rc_time_unit_blocks
};

struct rc_curve_params
{
   uint64_t        coeff_a = 0;
   uint64_t        coeff_b = 0;
   int64_t         coeff_d = 0;
   uint8_t         shift = 0;
};

struct rc_decay_params
{
   uint32_t        decay_per_time_unit = 0;
   uint8_t         decay_per_time_unit_denom_shift = 0;
};

struct rc_resource_params
{
   int8_t          time_unit = rc_time_unit_seconds;
   uint64_t        resource_unit = 0;

   int32_t         budget_per_time_unit = 0;

   uint64_t        max_pool_size = 0;

   rc_curve_params curve_params;
   rc_decay_params decay_params;
};

int64_t compute_rc_cost_of_resource(
   const rc_curve_params& curve_params,
   int64_t current_pool,
   int64_t resource_count );
int64_t compute_pool_decay(
   const rc_decay_params& decay_params,
   int64_t current_pool,
   uint32_t dt
   );

} } } // steem::plugins::rc

FC_REFLECT_ENUM( steem::plugins::rc::rc_time_unit_type, (rc_time_unit_seconds)(rc_time_unit_blocks) )
FC_REFLECT( steem::plugins::rc::rc_curve_params, (coeff_a)(coeff_b)(coeff_d)(shift) )
FC_REFLECT( steem::plugins::rc::rc_decay_params, (decay_per_time_unit)(decay_per_time_unit_denom_shift) )
FC_REFLECT( steem::plugins::rc::rc_resource_params,
   (time_unit)
   (resource_unit)
   (budget_per_time_unit)
   (max_pool_size)
   (curve_params)
   (decay_params)
   )

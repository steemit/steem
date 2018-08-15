#pragma once

#include <fc/reflect/reflect.hpp>
#include <cstdint>

// Data structures and functions for resource dynamics "in-flight" (i.e. in the absence of user (witness) tweaking)
// This header includes RESOURCES functionality only, no RC (price / price curve).

namespace steem { namespace plugins { namespace rc {

struct rd_decay_params
{
   uint32_t        decay_per_time_unit = 0;
   uint8_t         decay_per_time_unit_denom_shift = 0;
};

struct rd_dynamics_params
{
   uint64_t        resource_unit = 0;

   int32_t         budget_per_time_unit = 0;

   uint64_t        pool_eq = 0;
   uint64_t        max_pool_size = 0;

   rd_decay_params decay_params;
};

int64_t rd_compute_pool_decay(
   const rd_decay_params& decay_params,
   int64_t current_pool,
   uint32_t dt
   );

} } }

FC_REFLECT( steem::plugins::rc::rd_decay_params,
   (decay_per_time_unit)
   (decay_per_time_unit_denom_shift)
   )

FC_REFLECT( steem::plugins::rc::rd_dynamics_params,
   (resource_unit)
   (budget_per_time_unit)
   (pool_eq)
   (max_pool_size)
   (decay_params)
   )

#pragma once

#include <fc/reflect/reflect.hpp>
#include <cstdint>

// Data structures and functions for resource dynamics "in-flight" (i.e. in the absence of user (witness) tweaking)
// This header includes RESOURCES functionality only, no RC (price / price curve).

namespace steem { namespace chain { namespace util {

struct rd_decay_params
{
   uint32_t        decay_per_time_unit = 0;
   uint8_t         decay_per_time_unit_denom_shift = 0;

   bool operator==( const rd_decay_params& other )const
   {
      return std::tie( decay_per_time_unit, decay_per_time_unit_denom_shift )
          == std::tie( other.decay_per_time_unit, other.decay_per_time_unit_denom_shift );
   }

   bool operator!=( const rd_decay_params& other )const
   {  return !((*this) == other);   }
};

struct rd_dynamics_params
{
   uint64_t        resource_unit = 0;

   int32_t         budget_per_time_unit = 0;

   uint64_t        pool_eq = 0;
   uint64_t        max_pool_size = 0;

   rd_decay_params decay_params;

   int64_t         min_decay = 0;

   bool operator==( const rd_dynamics_params& other )const
   {
      return std::tie(
             resource_unit,
             budget_per_time_unit,
             pool_eq,
             max_pool_size,
             decay_params,
             min_decay )
          == std::tie(
             other.resource_unit,
             other.budget_per_time_unit,
             other.pool_eq,
             other.max_pool_size,
             other.decay_params,
             other.min_decay );
   }

   bool operator!=( const rd_dynamics_params& other )const
   {  return !((*this) == other);   }
};

int64_t rd_apply( const rd_dynamics_params& rd, int64_t pool );
int64_t rd_compute_pool_decay(
   const rd_decay_params& decay_params,
   int64_t current_pool,
   uint32_t dt
   );

} } }

FC_REFLECT( steem::chain::util::rd_decay_params,
   (decay_per_time_unit)
   (decay_per_time_unit_denom_shift)
   )

FC_REFLECT( steem::chain::util::rd_dynamics_params,
   (resource_unit)
   (budget_per_time_unit)
   (pool_eq)
   (max_pool_size)
   (decay_params)
   (min_decay)
   )

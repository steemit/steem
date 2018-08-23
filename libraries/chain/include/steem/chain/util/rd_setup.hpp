#pragma once

#include <fc/reflect/reflect.hpp>
#include <cstdint>

// It somewhat breaks the abstraction of chain/util as "pure" algorithms to include this here,
// but we can't keep the STEEM_RD_* constants in this header because they're used for validation and
// need to be available to steem::protocol.
#include <steem/protocol/config.hpp>

// Data structures and functions for runtime user (witness) input of resource dynamics parameters.
// This header includes RESOURCES functionality only, no RC (price / price curve).
// Includes adapter to set dynamics params based on user+system params

namespace steem { namespace chain { namespace util {

// Parameters settable by users.
struct rd_user_params
{
   int32_t         budget_per_time_unit = 0;
   uint32_t        decay_per_time_unit = 0;
};

// Parameters hard-coded in the system.
struct rd_system_params
{
   uint64_t        resource_unit = 0;
   uint32_t        decay_per_time_unit_denom_shift = STEEM_RD_DECAY_DENOM_SHIFT;
};

struct rd_dynamics_params;

void rd_validate_user_params(
   const rd_user_params& user_params );

void rd_setup_dynamics_params(
   const rd_user_params& user_params,
   const rd_system_params& system_params,
   rd_dynamics_params& dparams_out );

} } }

FC_REFLECT( steem::chain::util::rd_user_params,
   (budget_per_time_unit)
   (decay_per_time_unit)
   )

FC_REFLECT( steem::chain::util::rd_system_params,
   (resource_unit)
   (decay_per_time_unit_denom_shift)
   )

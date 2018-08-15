#pragma once

#include <fc/reflect/reflect.hpp>
#include <cstdint>

// Data structures and functions for runtime user (witness) input of resource dynamics parameters.
// This header includes RESOURCES functionality only, no RC (price / price curve).
// Includes adapter to set dynamics params based on user+system params

#define STEEM_RD_MIN_DECAY_BITS     6
#define STEEM_RD_MAX_DECAY_BITS    32
#define STEEM_RD_DECAY_DENOM_SHIFT 36
#define STEEM_RD_MAX_POOL_BITS     64
#define STEEM_RD_MAX_BUDGET_1      ((uint64_t(1) << (STEEM_RD_MAX_POOL_BITS + STEEM_RD_MIN_DECAY_BITS - STEEM_RD_DECAY_DENOM_SHIFT))-1)
#define STEEM_RD_MAX_BUDGET_2      ((uint64_t(1) << (64-STEEM_RD_DECAY_DENOM_SHIFT))-1)
#define STEEM_RD_MAX_BUDGET        ((STEEM_RD_MAX_BUDGET_1 < STEEM_RD_MAX_BUDGET_2) ? STEEM_RD_MAX_BUDGET_1 : STEEM_RD_MAX_BUDGET_2)
#define STEEM_RD_MIN_DECAY         (uint32_t(1) << STEEM_RD_MIN_DECAY_BITS)
#define STEEM_RD_MIN_BUDGET        1
#define STEEM_RD_MAX_DECAY         (uint32_t(0xFFFFFFFF))

namespace steem { namespace plugins { namespace rc {

// Parameters settable by users.
struct rd_user_params
{
   uint64_t        budget_per_time_unit = 0;
   uint32_t        decay_per_time_unit = 0;
};

// Parameters hard-coded in the system.
struct rd_system_params
{
   uint64_t        resource_unit = 0;
   uint32_t        decay_per_time_unit_denom_shift = STEEM_RD_DECAY_DENOM_SHIFT;
};

struct rd_dynamics_params;

void rd_setup_dynamics_params(
   const rd_user_params& user_params,
   const rd_system_params& system_params,
   rd_dynamics_params& dparams_out );

} } }

FC_REFLECT( steem::plugins::rc::rd_user_params,
   (budget_per_time_unit)
   (decay_per_time_unit)
   )

FC_REFLECT( steem::plugins::rc::rd_system_params,
   (resource_unit)
   (decay_per_time_unit_denom_shift)
   )

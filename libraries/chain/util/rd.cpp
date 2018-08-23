
#include <steem/chain/util/rd_setup.hpp>
#include <steem/chain/util/rd_dynamics.hpp>

#include <fc/uint128.hpp>
#include <fc/exception/exception.hpp>

// Implement the resource dynamics functions

// max_decay = (uint64_t(1) << (MAX_POOL_BITS + MIN_DECAY_BITS - DECAY_DENOM_SHIFT))-1;

namespace steem { namespace chain { namespace util {

void rd_validate_user_params(
   const rd_user_params& user_params )
{
   FC_ASSERT( user_params.budget_per_time_unit >= STEEM_RD_MIN_BUDGET );
   FC_ASSERT( user_params.budget_per_time_unit <= STEEM_RD_MAX_BUDGET );
   FC_ASSERT( user_params.decay_per_time_unit >= STEEM_RD_MIN_DECAY );
   FC_ASSERT( user_params.decay_per_time_unit <= STEEM_RD_MAX_DECAY );
}

int64_t rd_compute_pool_decay(
   const rd_decay_params& decay_params,
   int64_t current_pool,
   uint32_t dt
   )
{
   if( current_pool < 0 )
      return -rd_compute_pool_decay( decay_params, -current_pool, dt );

   fc::uint128_t decay_amount = uint64_t( decay_params.decay_per_time_unit ) * uint64_t( dt );
   decay_amount *= uint64_t( current_pool );
   decay_amount >>= decay_params.decay_per_time_unit_denom_shift;
   uint64_t result = decay_amount.to_uint64();
   return (
           (result > uint64_t( current_pool ))
           ? current_pool
           : int64_t(result)
          );
}

int64_t rd_apply( const rd_dynamics_params& rd, int64_t pool )
{
   // Apply resource dynamics, hardcoded dt=1
   int64_t decay = rd_compute_pool_decay( rd.decay_params, pool, 1 );
   int64_t budget = rd.budget_per_time_unit;
   int64_t max_pool_size = rd.max_pool_size;
   decay = std::max( decay, rd.min_decay );
   int64_t new_pool = pool + budget - decay;
   new_pool = std::min( new_pool, max_pool_size );
   new_pool = std::max( new_pool, int64_t(0) );
   return new_pool;
}

void rd_setup_dynamics_params(
   const rd_user_params& user_params,
   const rd_system_params& system_params,
   rd_dynamics_params& dparams_out )
{
   dparams_out.resource_unit = system_params.resource_unit;
   dparams_out.budget_per_time_unit = user_params.budget_per_time_unit;
   dparams_out.decay_params.decay_per_time_unit = user_params.decay_per_time_unit;
   dparams_out.decay_params.decay_per_time_unit_denom_shift = system_params.decay_per_time_unit_denom_shift;

   //
   // Following assert should never trigger, as decay_per_time_unit_denom_shift is a system param and should
   //   not be user settable.  System code which sets it should just set it to
   //   STEEM_RD_DECAY_DENOM_SHIFT.  The interface is designed to support other values of this
   //   parameter, but this implementation is not.  So for now, code that uses the interface just needs
   //   to accept there's currently exactly one acceptable integer value for this field.
   //
   // TLDR, if you trigger this FC_ASSERT(), you've created a bug.  You need to either re-write the caller
   //   code so that it sets decay_per_time_unit_denom_shift = STEEM_RD_DECAY_DENOM_SHIFT,
   //   or re-write this implementation so that the functionality here (including bounds checks) correctly
   //   handles the values you want to feed it.
   //
   FC_ASSERT( system_params.decay_per_time_unit_denom_shift == STEEM_RD_DECAY_DENOM_SHIFT );

   // Initialize pool_eq to the smallest integer where budget/decay are in balance
   // Let b=budget, d=decay_per_time_unit, s=decay_per_time_unit_shift
   // Balance of forces says:
   //
   //     (d*x) >> s >= b
   // iff  d*x       >= b << s
   // iff  x         >= (b << s) / d
   //
   // So we can set x = ceil((b << s) / d) = ((b << s) + (d-1)) / d.

   // Worst-case assert for the following is d=1, in which case b << s must be less than 2^64
   static_assert( STEEM_RD_MAX_BUDGET < (uint64_t(1) << (64-STEEM_RD_DECAY_DENOM_SHIFT)),
      "Computation of temp could overflow here, set smaller STEEM_RD_MAX_BUDGET" );

   fc::uint128_t temp = user_params.budget_per_time_unit;
   temp <<= system_params.decay_per_time_unit_denom_shift;
   temp += (user_params.decay_per_time_unit-1);
   temp /= user_params.decay_per_time_unit;
   dparams_out.pool_eq = temp.to_uint64();
   dparams_out.max_pool_size = dparams_out.pool_eq;

   // Debug code:  Check that the above reasoning is correct and we've set pool_eq
   //    to the smallest integer pool size where decay >= budget
   // TODO: Test that these asserts never trigger.  If they do, there is a bug in
   //       the code which computes pool_eq.
   //  - Check specific corner cases
   //  - Monte carlo large number of random inputs
   //  - AFL or the like?

   // Helper lambda to check whether decay >= budget at a given pool level
   auto check_pool_eq = []( const rd_dynamics_params& dparams, uint64_t pool ) -> bool
   {
      int64_t decay = rd_compute_pool_decay( dparams.decay_params, pool, 1 );
      int32_t budget = dparams.budget_per_time_unit;
      return (decay >= budget);
   };

   FC_ASSERT(  check_pool_eq( dparams_out, dparams_out.pool_eq   ) );
   FC_ASSERT( !check_pool_eq( dparams_out, dparams_out.pool_eq-1 ) );
}

} } }

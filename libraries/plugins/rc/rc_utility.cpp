
#include <steem/plugins/rc/rc_utility.hpp>

#include <fc/uint128.hpp>

namespace steem { namespace plugins { namespace rc {

using fc::uint128_t;

int64_t compute_rc_cost_of_resource(
   const rc_curve_params& curve_params,
   int64_t current_pool,
   int64_t resource_count )
{
   if( resource_count <= 0 )
   {
      if( resource_count < 0 )
         return -compute_rc_cost_of_resource( curve_params, current_pool, -resource_count );
      return 0;
   }
   uint128_t num = uint128_t( resource_count );
   num *= curve_params.coeff_a;
   uint128_t denom = uint128_t( curve_params.coeff_b );

   // Negative pool doesn't increase price beyond p_max
   //   i.e. define p(x) = p(0) for all x < 0
   denom += (current_pool > 0) ? uint64_t(current_pool) : uint64_t(0);
   uint128_t num_denom = num / denom;
   uint128_t discount = uint128_t( resource_count );
   discount *= curve_params.coeff_d;
   // Clamp A / (B+x) - D to 1 in case D is too big
   if( num_denom <= discount )
      return 1;
   uint128_t num_denom_minus_discount = (num_denom - discount).to_uint64();
   num_denom_minus_discount >>= curve_params.shift;
   // Add 1 to avoid 0 result in case of various rounding issues,
   // err on the side of rounding not in the user's favor
   return num_denom_minus_discount.to_uint64()+1;
}

int64_t compute_pool_decay(
   const rc_decay_params& decay_params,
   int64_t current_pool,
   uint32_t dt
   )
{
   if( current_pool < 0 )
      return -compute_pool_decay( decay_params, -current_pool, dt );

   uint128_t decay_amount = uint64_t( decay_params.decay_per_time_unit ) * uint64_t( dt );
   decay_amount *= uint64_t( current_pool );
   decay_amount >>= decay_params.decay_per_time_unit_denom_shift;
   uint64_t result = decay_amount.to_uint64();
   return (
           (result > uint64_t( current_pool ))
           ? current_pool
           : int64_t(result)
          );
}

} } }

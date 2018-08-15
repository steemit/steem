
#include <steem/plugins/rc/rc_utility.hpp>

#include <fc/reflect/variant.hpp>
#include <fc/uint128.hpp>

namespace steem { namespace plugins { namespace rc {

using fc::uint128_t;

int64_t compute_rc_cost_of_resource(
   const rc_price_curve_params& curve_params,
   int64_t current_pool,
   int64_t resource_count,
   int64_t rc_regen
   )
{
   /*
   ilog( "compute_rc_cost_of_resources( ${params}, ${pool}, ${res}, ${reg} )",
      ("params", curve_params)
      ("pool", current_pool)
      ("res", resource_count)
      ("reg", rc_regen) );
   */
   FC_ASSERT( rc_regen > 0 );

   if( resource_count <= 0 )
   {
      if( resource_count < 0 )
         return -compute_rc_cost_of_resource( curve_params, current_pool, -resource_count, rc_regen );
      return 0;
   }
   uint128_t num = uint128_t( rc_regen );
   num *= curve_params.coeff_a;
   // rc_regen * coeff_a is already risking overflowing 128 bits,
   //   so shift it immediately before multiplying by resource_count
   num >>= curve_params.shift;
   // err on the side of rounding not in the user's favor
   num += 1;
   num *= uint64_t( resource_count );

   uint128_t denom = uint128_t( curve_params.coeff_b );

   // Negative pool doesn't increase price beyond p_max
   //   i.e. define p(x) = p(0) for all x < 0
   denom += (current_pool > 0) ? uint64_t(current_pool) : uint64_t(0);
   uint128_t num_denom = num / denom;
   // Add 1 to avoid 0 result in case of various rounding issues,
   // err on the side of rounding not in the user's favor
   // ilog( "result: ${r}", ("r", num_denom.to_uint64()+1) );
   return num_denom.to_uint64()+1;
}

} } }

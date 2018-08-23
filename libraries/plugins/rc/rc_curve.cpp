#include "rc_curve.hpp"

#include <steem/protocol/config.hpp>

#include <fc/exception/exception.hpp>
#include <fc/log/logger.hpp>

#include <math.h>
#include <limits>

namespace steem { namespace plugins { namespace rc {

template< typename T >
T check_and_cast( double num )
{
   FC_ASSERT( num >= std::numeric_limits< T >::min() && num <= std::numeric_limits< T >::max(),
      "Unexpected value out of range. value: ${v} type: ${t}", ("v", num)("t", fc::get_typename< T >::name()) );

   return (T)num;
}

rc_resource_params generate_rc_curve( const rc_curve_gen_params& params )
{
   try
   {
      const static double log_2 = std::log( 2.0 );

      rc_resource_params result;

      result.time_unit = params.time_unit;

      // Convert time unit to seconds. 1 for seconds, 3 (block interval) for blocks
      uint32_t time_unit_sec = params.time_unit == rc_time_unit_seconds ? 1 : STEEM_BLOCK_INTERVAL;
      fc::microseconds budget_time = params.budget_time;
      fc::microseconds half_life = params.half_life;
      double inelasticity_threshold = double( params.inelasticity_threshold_num ) / double( params.inelasticity_threshold_denom );

      double tau = half_life.to_seconds() / log_2;
      double decay_per_sec_float = -1 * std::expm1( -1.0 / tau );

      double budget_per_sec = double( params.budget ) / double( budget_time.to_seconds() );

      uint32_t resource_unit_exponent = 0;
      if( params.resource_unit_exponent )
      {
         resource_unit_exponent = *(params.resource_unit_exponent);
      }
      else
      {
         double pool_eq = budget_per_sec / decay_per_sec_float;
         double resource_unit_exponent_float = std::log( params.small_stockpile_size / pool_eq ) / std::log( params.resource_unit_base );
         resource_unit_exponent = std::max( 0, check_and_cast< int32_t >( std::ceil( resource_unit_exponent_float ) ) );
      }

      result.resource_unit = check_and_cast< uint64_t >( std::pow( params.resource_unit_base, resource_unit_exponent ) );

      budget_per_sec *= result.resource_unit;
      double budget_per_time_unit = budget_per_sec * time_unit_sec;
      result.budget_per_time_unit = check_and_cast< int32_t >( budget_per_time_unit + 0.5 );

      double pool_eq = budget_per_sec / decay_per_sec_float;
      result.pool_eq = check_and_cast< uint64_t >( pool_eq + 1 );
      result.max_pool_size = check_and_cast< uint64_t >( ( pool_eq * 2.0 ) + 0.5 );

      double a_point = (double)( params.a_point_num ) / (double)( params.a_point_denom );
      double u_point = (double)( params.u_point_num ) / (double)( params.u_point_denom );
      double k = u_point / ( a_point * ( 1.0 - u_point ) );

      double B = inelasticity_threshold * pool_eq;
      double A = tau / k;

      if( A < 1.0 || B < 1.0 )
      {
         wlog( "Bad parameter vlaue (is time too short?)" );
         FC_ASSERT( false, "Bad parameter vlaue (is time too short?) A: {$A} B: {$B} Params: ${P}", ("A", A)("B", B)("P", params) );
      }

      double curve_shift_float = std::log( ((uint64_t)UINT64_MAX) / A ) / log_2;
      uint64_t curve_shift = check_and_cast< uint64_t >( std::floor( curve_shift_float ) );
      result.curve_params.coeff_a = check_and_cast< uint64_t >( A * ( std::pow( 2, curve_shift ) + 0.5 ) );
      result.curve_params.coeff_b = check_and_cast< uint64_t >( B + 0.5 );
      result.curve_params.shift = check_and_cast< uint8_t >( curve_shift );

      double decay_per_time_unit_float = -1 * std::expm1( -1 * ( time_unit_sec * log_2 ) / half_life.to_seconds() );

      double decay_per_time_unit_denom_shift_float = std::log( ((uint64_t)UINT32_MAX) / decay_per_time_unit_float ) / log_2;
      uint64_t decay_per_time_unit_denom_shift = check_and_cast< uint64_t >( std::floor( decay_per_time_unit_denom_shift_float ) );
      uint64_t decay_per_time_unit = check_and_cast< uint64_t >( decay_per_time_unit_float * std::pow( 2.0, decay_per_time_unit_denom_shift ) + 0.5 );

      result.decay_params.decay_per_time_unit = decay_per_time_unit;
      result.decay_params.decay_per_time_unit_denom_shift = decay_per_time_unit_denom_shift;

      return result;
   }
   FC_CAPTURE_LOG_AND_RETHROW( (params) )
}

} } } //steed::plugins::rc

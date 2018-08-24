
#include <steem/plugins/rc/rc_curve.hpp>
#include <steem/protocol/config.hpp>

#include <fc/exception/exception.hpp>
#include <fc/log/logger.hpp>
#include <fc/uint128.hpp>

#include <cstdint>

namespace steem { namespace plugins { namespace rc {

void rc_curve_gen_params::validate()const
{
   FC_ASSERT( inelasticity_threshold_num     <=  UINT16_MAX );
   FC_ASSERT( inelasticity_threshold_denom   <=  UINT16_MAX );
   FC_ASSERT( a_point_num                    <=  UINT16_MAX );
   FC_ASSERT( a_point_denom                  <=  UINT16_MAX );
   FC_ASSERT( u_point_num                    <=  UINT16_MAX );
   FC_ASSERT( u_point_denom                  <=  UINT16_MAX );

   FC_ASSERT( inelasticity_threshold_num > 0 );
   FC_ASSERT( inelasticity_threshold_denom > inelasticity_threshold_num );

   FC_ASSERT( a_point_num > 0 );
   FC_ASSERT( a_point_denom > a_point_num );

   FC_ASSERT( u_point_num > 0 );
   FC_ASSERT( u_point_denom > u_point_num );
}

void generate_rc_curve_params(
   rc_price_curve_params& price_curve_params,
   const rd_dynamics_params& resource_dynamics_params,
   const rc_curve_gen_params& curve_gen_params )
{
   // Fillin curve_params based on computations

   // B = inelasticity_threshold * pool_eq
   // A = tau / k
   // tau = characteristic time
   //
   // -> (1-r)^tau = 1/e
   //
   // where r = decay_per_time_unit / 2^decay_per_time_unit_denom_shift
   //
   // -> tau log(1-r) = log(1/e) = -log(e) = -1
   // -> tau = -1 / log(1-r)
   //        = -1 / log1p(-r)
   //        ~  1 / r
   //        = N / decay_per_time_unit
   // where N = 2^decay_per_time_unit_denom_shift
   //
   //
   // k = u / (a * (1-u))
   // -> A = tau / k = tau * 1/k
   //      ~ (1/r) / (u/(a*(1-u)))
   //      = a*(1-u) / (r*u)
   //      = (a - a*u) / (r*u)
   //      = a / (r*u) - a/r
   //
   //      = (a_point_num / a_point_denom) / ((decay_per_time_unit / N) * (u_point_num / u_point_denom)) - (a_point_num / a_point_denom) / (decay_per_time_unit / N)
   //      = (a_point_num * N * u_point_denom) / (a_point_denom * decay_per_time_unit * u_point_num ) - (a_point_num * N) / (a_point_denom * decay_per_time_unit)
   //      = (a_point_num * N * u_point_denom - a_point_num * N * u_point_num) / (a_point_denom * decay_per_time_unit * u_point_num)
   //

   FC_ASSERT( resource_dynamics_params.decay_params.decay_per_time_unit > 0 );

   uint64_t A_num = curve_gen_params.a_point_num * ( curve_gen_params.u_point_denom - curve_gen_params.u_point_num );
   uint64_t A_denom = curve_gen_params.a_point_denom * curve_gen_params.u_point_num * resource_dynamics_params.decay_params.decay_per_time_unit;

   // A_denom still needs to be multiplied by N
   A_num = std::max( A_num, uint64_t(1) );
   A_denom = std::max( A_denom, uint64_t(1) );

   // How many places are needed to shift A_num left until leading 1 bit is in the top bit position of uint64_t?
   uint8_t shift1 = 63 - boost::multiprecision::detail::find_msb( A_num );
   uint64_t A_1 = (A_num << shift1) / A_denom;
   // n.b. A_1 cannot be smaller than 2^31 because we are dividing a number at least 2^63 by uint32_t
   // How many places are needed to shift A_1 left until leading 1 bit is in the top bit position of uint64_t?
   uint8_t shift2 = 63 - boost::multiprecision::detail::find_msb( A_1 );
   fc::uint128_t u128_A = A_num;
   uint8_t shift = shift1+shift2;
   FC_ASSERT( shift >= resource_dynamics_params.decay_params.decay_per_time_unit_denom_shift );
   u128_A <<= shift;
   u128_A /= A_denom;
   price_curve_params.coeff_a = u128_A.to_uint64();

   // We left N out of the numerator, we can effectively multiply it by subtracting it from the number of bits to shift right
   price_curve_params.shift = uint8_t( shift - resource_dynamics_params.decay_params.decay_per_time_unit_denom_shift );
   // We now know how much to shift A_num so that it's 64 bits, the only thing left to do is calculate A
   // at 128-bit precision so we know what the low bits are.

   fc::uint128_t u128_B = resource_dynamics_params.pool_eq;
   u128_B *= curve_gen_params.inelasticity_threshold_num;
   u128_B /= curve_gen_params.inelasticity_threshold_denom;
   price_curve_params.coeff_b = u128_B.to_uint64();
}

} } } //steed::plugins::rc

/**
 * Note:
 *
 * This header is contained within a source directory because it is not meant to be used anywhere
 * EXCEPT the rc plugin. The implementation uses floating point arithmetic which can be
 * non-deterministic. This implementation is NOT suitable for a consensus level protocol.
 */

#pragma once
#include <steem/plugins/rc/rc_utility.hpp>

// This header shouldn't be needed, but fc reflection was not working
// and something in here makes it work.
#include <steem/protocol/types.hpp>

#include <fc/time.hpp>

namespace steem { namespace plugins { namespace rc {

/**
 * The design of these classes is to be json compatible with the parameters
 * defined for use with rc_generate_resource_parameters.py
 *
 * Some of the structures are redundant compared to existing C++ types but
 * are designed for this purpose.
 */

struct rc_time_type
{
   uint32_t seconds  = 0;
   uint32_t minutes  = 0;
   uint32_t hours    = 0;
   uint32_t days     = 0;
   uint32_t weeks    = 0;
   uint32_t months   = 0;
   uint32_t years    = 0;

   operator fc::microseconds() const
   {
      return fc::seconds( seconds )
         + fc::minutes( minutes )
         + fc::hours( hours )
         + fc::days( days )
         + fc::days( weeks * 7 )
         + fc::days( months * 30 )
         + fc::days( years * 365 );
   }
};

struct rc_curve_gen_params
{
   rc_time_unit_type       time_unit = rc_time_unit_seconds;
   rc_time_type            budget_time;
   uint64_t                budget                        = 0;
   rc_time_type            half_life;
   uint32_t                inelasticity_threshold_num    = 1;
   uint32_t                inelasticity_threshold_denom  = 128;
   uint64_t                small_stockpile_size          = (1ull << 32);
   uint32_t                resource_unit_base            = 10;
   fc::optional< int32_t > resource_unit_exponent;
   uint32_t                a_point_num                   = 1;
   uint32_t                a_point_denom                 = 32;
   uint32_t                u_point_num                   = 1;
   uint32_t                u_point_denom                 = 2;
};

rc_resource_params generate_rc_curve( const rc_curve_gen_params& params );

} } } // steem::plugins::rc

FC_REFLECT( steem::plugins::rc::rc_time_type,
   (seconds)
   (minutes)
   (hours)
   (days)
   (weeks)
   (months)
   (years)
   )

FC_REFLECT( steem::plugins::rc::rc_curve_gen_params,
   (time_unit)
   (budget_time)
   (budget)
   (half_life)
   (inelasticity_threshold_num)
   (inelasticity_threshold_denom)
   (small_stockpile_size)
   (resource_unit_base)
   (resource_unit_exponent)
   (a_point_num)
   (a_point_denom)
   (u_point_num)
   (u_point_denom)
   )

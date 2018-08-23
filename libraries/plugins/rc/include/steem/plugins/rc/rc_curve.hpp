
#pragma once
#include <steem/plugins/rc/rc_utility.hpp>

#include <steem/protocol/types.hpp>

#include <fc/time.hpp>

namespace steem { namespace plugins { namespace rc {

struct rc_curve_gen_params
{
   uint16_t                inelasticity_threshold_num    = 1;
   uint16_t                inelasticity_threshold_denom  = 128;
   uint16_t                a_point_num                   = 1;
   uint16_t                a_point_denom                 = 32;
   uint16_t                u_point_num                   = 1;
   uint16_t                u_point_denom                 = 2;

   void validate()const;
};

void generate_rc_curve_params(
   rc_price_curve_params& price_curve_params,
   const rd_dynamics_params& resource_dynamics_params,
   const rc_curve_gen_params& curve_gen_params
   );

} } } // steem::plugins::rc

FC_REFLECT( steem::plugins::rc::rc_curve_gen_params,
   (inelasticity_threshold_num)
   (inelasticity_threshold_denom)
   (a_point_num)
   (a_point_denom)
   (u_point_num)
   (u_point_denom)
   )

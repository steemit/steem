
#pragma once
#include <dpn/plugins/rc/rc_utility.hpp>

#include <dpn/protocol/types.hpp>

#include <fc/time.hpp>

namespace dpn { namespace plugins { namespace rc {

struct rc_curve_gen_params
{
   uint64_t                inelasticity_threshold_num    = 1;
   uint64_t                inelasticity_threshold_denom  = 128;
   uint64_t                a_point_num                   = 37;
   uint64_t                a_point_denom                 = 1000;
   uint64_t                u_point_num                   = 1;
   uint64_t                u_point_denom                 = 2;

   void validate()const;
};

void generate_rc_curve_params(
   rc_price_curve_params& price_curve_params,
   const rd_dynamics_params& resource_dynamics_params,
   const rc_curve_gen_params& curve_gen_params
   );

} } } // dpn::plugins::rc

FC_REFLECT( dpn::plugins::rc::rc_curve_gen_params,
   (inelasticity_threshold_num)
   (inelasticity_threshold_denom)
   (a_point_num)
   (a_point_denom)
   (u_point_num)
   (u_point_denom)
   )

#pragma once

#include <dpn/protocol/types.hpp>

#include <dpn/plugins/block_data_export/exportable_block_data.hpp>
#include <dpn/plugins/rc/resource_count.hpp>

#include <fc/int_array.hpp>

#include <vector>

namespace dpn { namespace plugins { namespace rc {

using dpn::plugins::block_data_export::exportable_block_data;
using dpn::protocol::account_name_type;

struct rc_transaction_info
{
   account_name_type         resource_user;
   count_resources_result    usage;
   fc::int_array< int64_t, DPN_NUM_RESOURCE_TYPES >
                             cost;
};

typedef rc_transaction_info rc_optional_action_info;

struct rc_block_info
{
   resource_count_type       dt;
   resource_count_type       decay;
   resource_count_type       budget;
   resource_count_type       usage;
   resource_count_type       adjustment;
   resource_count_type       pool;
};

struct exp_rc_data
   : public exportable_block_data
{
   exp_rc_data();
   virtual ~exp_rc_data();

   virtual void to_variant( fc::variant& v )const override;

   rc_block_info                          block_info;
   std::vector< rc_transaction_info >     tx_info;
   std::vector< rc_optional_action_info > opt_action_info;
};

} } } // dpn::plugins::rc

FC_REFLECT( dpn::plugins::rc::rc_transaction_info,
   (resource_user)
   (usage)
   (cost)
)

FC_REFLECT( dpn::plugins::rc::rc_block_info,
   (dt)
   (decay)
   (budget)
   (usage)
   (adjustment)
   (pool)
)

FC_REFLECT( dpn::plugins::rc::exp_rc_data,
   (block_info)
   (tx_info)
   (opt_action_info)
)

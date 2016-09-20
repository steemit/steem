#pragma once

#include <steemit/chain/protocol/operations.hpp>
#include <steemit/chain/protocol/types.hpp>

namespace steemit { namespace app {

struct full_operation_object
{
   full_operation_object();
   full_operation_object( const steemit::chain::operation_object& op_obj );

   steemit::chain::transaction_id_type trx_id;
   uint32_t                            block = 0;
   uint32_t                            trx_in_block = 0;
   uint16_t                            op_in_trx = 0;
   uint64_t                            virtual_op = 0;
   fc::time_point_sec                  timestamp;
   steemit::chain::operation           op;
};

} }

FC_REFLECT( steemit::app::full_operation_object,
   (trx_id)
   (block)
   (trx_in_block)
   (op_in_trx)
   (virtual_op)
   (timestamp)
   (op)
)

#include <golos/plugins/operation_history/applied_operation.hpp>

namespace golos { namespace plugins { namespace operation_history {

    applied_operation::applied_operation() = default;

    applied_operation::applied_operation(const operation_object& op_obj)
        : trx_id(op_obj.trx_id),
          block(op_obj.block),
          trx_in_block(op_obj.trx_in_block),
          op_in_trx(op_obj.op_in_trx),
          virtual_op(op_obj.virtual_op),
          timestamp(op_obj.timestamp),
          op(fc::raw::unpack<protocol::operation>(op_obj.serialized_op)) {
    }

} } } // golos::plugins::operation_history

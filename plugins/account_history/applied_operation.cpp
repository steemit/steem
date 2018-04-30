#include <golos/plugins/account_history/applied_operation.hpp>

namespace golos { namespace plugins { namespace account_history {

    applied_operation::applied_operation() {
    }

    applied_operation::applied_operation(const operation_object &op_obj) : trx_id(op_obj.trx_id),
            block(op_obj.block), trx_in_block(op_obj.trx_in_block), op_in_trx(op_obj.op_in_trx),
            virtual_op(op_obj.virtual_op), timestamp(op_obj.timestamp) {
        //fc::raw::unpack( op_obj.serialized_op, op );     // g++ refuses to compile this as ambiguous
        op = fc::raw::unpack<protocol::operation>(op_obj.serialized_op);
    }

} } } // golos::plugins::account_history

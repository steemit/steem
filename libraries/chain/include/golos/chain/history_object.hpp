#pragma once

#include <golos/protocol/authority.hpp>
#include <golos/protocol/operations.hpp>
#include <golos/protocol/steem_operations.hpp>

#include <golos/chain/steem_object_types.hpp>
#include <golos/chain/witness_objects.hpp>

#include <boost/multi_index/composite_key.hpp>


namespace golos {
    namespace chain {

        class operation_object
                : public object<operation_object_type, operation_object> {
        public:
            operation_object() = delete;

            template<typename Constructor, typename Allocator>
            operation_object(Constructor &&c, allocator <Allocator> a)
                    :serialized_op(a.get_segment_manager()) {
                c(*this);
            }

            id_type id;

            transaction_id_type trx_id;
            uint32_t block = 0;
            uint32_t trx_in_block = 0;
            uint16_t op_in_trx = 0;
            uint64_t virtual_op = 0;
            time_point_sec timestamp;
            buffer_type serialized_op;
        };

        struct by_location;
        struct by_transaction_id;
        typedef multi_index_container <
        operation_object,
        indexed_by<
                ordered_unique < tag <
                by_id>, member<operation_object, operation_id_type, &operation_object::id>>,
        ordered_unique <tag<by_location>,
        composite_key<operation_object,
                member < operation_object, uint32_t, &operation_object::block>,
        member<operation_object, uint32_t, &operation_object::trx_in_block>,
        member<operation_object, uint16_t, &operation_object::op_in_trx>,
        member<operation_object, uint64_t, &operation_object::virtual_op>,
        member<operation_object, operation_id_type, &operation_object::id>
        >
        >,
        ordered_unique <tag<by_transaction_id>,
        composite_key<operation_object,
                member <
                operation_object, transaction_id_type, &operation_object::trx_id>,
        member<operation_object, operation_id_type, &operation_object::id>
        >
        >
        >,
        allocator <operation_object>
        >
        operation_index;

    }
}

FC_REFLECT((golos::chain::operation_object), (id)(trx_id)(block)(trx_in_block)(op_in_trx)(virtual_op)(timestamp)(serialized_op))
CHAINBASE_SET_INDEX_TYPE(golos::chain::operation_object, golos::chain::operation_index)

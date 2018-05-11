#pragma once

#include <golos/protocol/authority.hpp>
#include <golos/protocol/operations.hpp>
#include <golos/protocol/steem_operations.hpp>

#include <chainbase/chainbase.hpp>

#include <golos/chain/index.hpp>
#include <golos/chain/steem_object_types.hpp>

#include <boost/multi_index/composite_key.hpp>

//
// Plugins should #define their SPACE_ID's so plugins with
// conflicting SPACE_ID assignments can be compiled into the
// same binary (by simply re-assigning some of the conflicting #defined
// SPACE_ID's in a build script).
//
// Assignment of SPACE_ID's cannot be done at run-time because
// various template automagic depends on them being known at compile
// time.
//

#ifndef OPERATION_HISTORY_SPACE_ID
#define OPERATION_HISTORY_SPACE_ID 14
#endif


namespace golos { namespace plugins { namespace operation_history {

    enum account_object_types {
        operation_object_type = (OPERATION_HISTORY_SPACE_ID << 8),
    };

    using namespace golos::chain;
    using namespace chainbase;

    class operation_object final: public object<operation_object_type, operation_object> {
    public:
        operation_object() = delete;

        template<typename Constructor, typename Allocator>
        operation_object(Constructor &&c, allocator <Allocator> a)
            : serialized_op(a.get_segment_manager()) {
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

    using operation_id_type = object_id<operation_object>;

    struct by_location;
    struct by_transaction_id;
    using operation_index = multi_index_container<
        operation_object,
        indexed_by<
            ordered_unique<
                tag<by_id>,
                member<operation_object, operation_id_type, &operation_object::id>>,
            ordered_unique<
                tag<by_location>,
                composite_key<
                    operation_object,
                    member<operation_object, uint32_t, &operation_object::block>,
                    member<operation_object, uint32_t, &operation_object::trx_in_block>,
                    member<operation_object, uint16_t, &operation_object::op_in_trx>,
                    member<operation_object, uint64_t, &operation_object::virtual_op>,
                    member<operation_object, operation_id_type, &operation_object::id>>>,
            ordered_unique<
                tag<by_transaction_id>,
                composite_key<
                    operation_object,
                    member<operation_object, transaction_id_type, &operation_object::trx_id>,
                    member<operation_object, operation_id_type, &operation_object::id>>>>,
        allocator<operation_object>>;

} } } // golos::plugins::operation_history

CHAINBASE_SET_INDEX_TYPE(
    golos::plugins::operation_history::operation_object,
    golos::plugins::operation_history::operation_index)


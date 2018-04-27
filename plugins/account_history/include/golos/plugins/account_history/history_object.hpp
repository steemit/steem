#pragma once

#include <golos/protocol/authority.hpp>
#include <golos/protocol/operations.hpp>
#include <golos/protocol/steem_operations.hpp>
#include <chainbase/chainbase.hpp>
#include <golos/chain/index.hpp>

#include <golos/chain/steem_object_types.hpp>
#include <golos/chain/witness_objects.hpp>

#include <boost/multi_index/composite_key.hpp>
#include <golos/plugins/account_history/applied_operation.hpp>

#ifndef ACCOUNT_HISTORY_SPACE_ID
#define ACCOUNT_HISTORY_SPACE_ID 12
#endif


namespace golos {
namespace plugins {
namespace account_history {

    enum account_object_types {
        account_history_object_type = (ACCOUNT_HISTORY_SPACE_ID << 8),
        operation_object_type = (ACCOUNT_HISTORY_SPACE_ID << 8) + 1,
    };

    using namespace golos::chain;
    using namespace chainbase;


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

    using operation_id_type = object_id<operation_object>;

    struct by_location;
    struct by_transaction_id;
    using operation_index = multi_index_container <
        operation_object,
            indexed_by<
                ordered_unique <
                    tag < by_id >,
                    member <
                        operation_object,
                        operation_id_type,
                        &operation_object::id
                        >
                    >,
                ordered_unique <
                    tag < by_location >,
                    composite_key<
                        operation_object,
                        member <
                            operation_object,
                            uint32_t,
                            &operation_object::block
                        >,
                        member<
                            operation_object,
                            uint32_t,
                            &operation_object::trx_in_block
                        >,
                        member<
                            operation_object,
                            uint16_t,
                            &operation_object::op_in_trx
                        >,
                        member<
                            operation_object,
                            uint64_t,
                            &operation_object::virtual_op
                        >,
                        member<
                            operation_object,
                            operation_id_type,
                            &operation_object::id
                        >
                    >
                >,
                ordered_unique <
                    tag <by_transaction_id>,
                    composite_key<
                        operation_object,
                        member <
                            operation_object,
                            transaction_id_type,
                            &operation_object::trx_id
                        >,
                        member <
                            operation_object,
                            operation_id_type,
                            &operation_object::id
                        >
                    >
                >
            >,
            allocator <operation_object>
        >;


    class account_history_object
            : public object<account_history_object_type, account_history_object> {
    public:
        template<typename Constructor, typename Allocator>
        account_history_object(Constructor &&c, allocator <Allocator> a) {
            c(*this);
        }

        id_type id;

        account_name_type account;
        uint32_t sequence = 0;
        operation_id_type op;
    };

    using account_history_id_type = object_id<account_history_object>;

    struct by_account;
    using account_history_index = multi_index_container <
        account_history_object,
            indexed_by<
                    ordered_unique <
                        tag < by_id>,
                        member<
                            account_history_object,
                            account_history_id_type,
                            &account_history_object::id
                        >
                    >,
                    ordered_unique <tag<by_account>,
                    composite_key<account_history_object,
                    member <
                        account_history_object, account_name_type, &account_history_object::account>,
                        member<account_history_object, uint32_t, &account_history_object::sequence>
                    >,
                    composite_key_compare <std::less<account_name_type>, std::greater<uint32_t>>
                >
            >,
            allocator <account_history_object>
        >
    ;

}
}
}

FC_REFLECT((golos::plugins::account_history::operation_object), (id)(trx_id)(block)(trx_in_block)(op_in_trx)(virtual_op)(timestamp)(serialized_op))
CHAINBASE_SET_INDEX_TYPE(golos::plugins::account_history::operation_object, golos::plugins::account_history::operation_index)

FC_REFLECT( (golos::plugins::account_history::account_history_object), (id)(account)(sequence)(op) )
CHAINBASE_SET_INDEX_TYPE( golos::plugins::account_history::account_history_object, golos::plugins::account_history::account_history_index)


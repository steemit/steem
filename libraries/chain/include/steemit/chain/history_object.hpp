#pragma once

#include <steemit/chain/protocol/authority.hpp>
#include <steemit/chain/protocol/types.hpp>
#include <steemit/chain/protocol/operations.hpp>
#include <steemit/chain/protocol/steem_operations.hpp>
#include <steemit/chain/witness_objects.hpp>

#include <graphene/db/generic_index.hpp>

#include <boost/multi_index/composite_key.hpp>


namespace steemit {
    namespace chain {

        using namespace graphene::db;

        class operation_object : public abstract_object<operation_object> {
        public:
            static const uint8_t space_id = implementation_ids;
            static const uint8_t type_id = impl_operation_object_type;

            transaction_id_type trx_id;
            uint32_t block = 0;
            uint32_t trx_in_block = 0;
            uint16_t op_in_trx = 0;
            uint64_t virtual_op = 0;
            time_point_sec timestamp;
            operation op;
        };

        struct by_location;
        struct by_transaction_id;
        typedef multi_index_container<
                operation_object,
                indexed_by<
                        ordered_unique<tag<by_id>, member<object, object_id_type, &object::id>>,
                        ordered_unique<tag<by_location>,
                                composite_key<operation_object,
                                        member<operation_object, uint32_t, &operation_object::block>,
                                        member<operation_object, uint32_t, &operation_object::trx_in_block>,
                                        member<operation_object, uint16_t, &operation_object::op_in_trx>,
                                        member<operation_object, uint64_t, &operation_object::virtual_op>,
                                        member<object, object_id_type, &operation_object::id>
                                >
                        >,
                        ordered_unique<tag<by_transaction_id>,
                                composite_key<operation_object,
                                        member<operation_object, transaction_id_type, &operation_object::trx_id>,
                                        member<object, object_id_type, &operation_object::id>
                                >
                        >
                >
        > operation_multi_index_type;

        class account_history_object
                : public abstract_object<account_history_object> {
        public:
            static const uint8_t space_id = implementation_ids;
            static const uint8_t type_id = impl_account_history_object_type;

            string account;
            uint32_t sequence = 0;
            operation_id_type op;
        };

        struct by_account;
        typedef multi_index_container<
                account_history_object,
                indexed_by<
                        ordered_unique<tag<by_id>, member<object, object_id_type, &object::id>>,
                        ordered_unique<tag<by_account>,
                                composite_key<account_history_object,
                                        member<account_history_object, string, &account_history_object::account>,
                                        member<account_history_object, uint32_t, &account_history_object::sequence>
                                >,
                                composite_key_compare<std::less<string>, std::greater<uint32_t>>
                        >
                >
        > account_history_multi_index_type;

        typedef generic_index<operation_object, operation_multi_index_type> operation_index;
        typedef generic_index<account_history_object, account_history_multi_index_type> account_history_index;
    }
}

FC_REFLECT_DERIVED(steemit::chain::operation_object, (graphene::db::object), (trx_id)(block)(trx_in_block)(op_in_trx)(virtual_op)(timestamp)(op))
FC_REFLECT_DERIVED(steemit::chain::account_history_object, (graphene::db::object), (account)(sequence)(op))

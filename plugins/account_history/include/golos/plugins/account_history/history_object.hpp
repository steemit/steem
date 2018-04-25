#pragma once

#include <golos/protocol/authority.hpp>
#include <golos/protocol/operations.hpp>
#include <golos/protocol/steem_operations.hpp>
#include <chainbase/chainbase.hpp>


#include <golos/chain/steem_object_types.hpp>
#include <golos/chain/witness_objects.hpp>

#include <boost/multi_index/composite_key.hpp>
#include <golos/plugins/account_history/plugin.hpp>

namespace golos {
    namespace plugins {
        namespace account_history {

            using namespace golos::chain;
            using namespace chainbase;

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

            struct by_account;
            typedef multi_index_container <
            account_history_object,
            indexed_by<
                    ordered_unique < tag <
                    by_id>, member<account_history_object, account_history_id_type, &account_history_object::id>>,
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
            account_history_index;

        }
    }
}

FC_REFLECT( (golos::plugins::account_history::account_history_object), (id)(account)(sequence)(op) )
CHAINBASE_SET_INDEX_TYPE( golos::plugins::account_history::account_history_object, golos::plugins::account_history::account_history_index)

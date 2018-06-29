#pragma once

#include <golos/protocol/authority.hpp>
#include <golos/protocol/operations.hpp>
#include <golos/protocol/steem_operations.hpp>

#include <chainbase/chainbase.hpp>

#include <golos/chain/index.hpp>
#include <golos/chain/steem_object_types.hpp>

#include <golos/plugins/operation_history/history_object.hpp>

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

#ifndef ACCOUNT_HISTORY_SPACE_ID
#define ACCOUNT_HISTORY_SPACE_ID 12
#endif


namespace golos { namespace plugins { namespace account_history {

    enum account_object_types {
        account_history_object_type = (ACCOUNT_HISTORY_SPACE_ID << 8)
    };

    using namespace golos::chain;
    using namespace chainbase;

    using golos::plugins::operation_history::operation_id_type;

    class account_history_object final: public object<account_history_object_type, account_history_object> {
    public:
        template <typename Constructor, typename Allocator>
        account_history_object(Constructor &&c, allocator <Allocator> a) {
            c(*this);
        }

        id_type id;

        account_name_type account;
        uint32_t block = 0;
        uint32_t sequence = 0;
        operation_id_type op;
    };

    using account_history_id_type = object_id<account_history_object>;

    struct by_location;
    struct by_account;
    using account_history_index = multi_index_container<
        account_history_object,
        indexed_by<
            ordered_unique<
                tag<by_id>,
                member<account_history_object, account_history_id_type, &account_history_object::id>>,
//            ordered_unique<
//                tag<by_location>,
//                composite_key<
//                    account_history_object,
//                    member<account_history_object, uint32_t, &account_history_object::block>>>,
            ordered_unique<
                tag<by_account>,
                composite_key<account_history_object,
                    member<account_history_object, account_name_type, &account_history_object::account>,
                    //member<account_history_object, uint32_t, &account_history_object::block>,
                    member<account_history_object, uint32_t, &account_history_object::sequence>>,
                composite_key_compare<std::less<account_name_type>, std::greater<uint32_t>>>>,
        allocator<account_history_object>>;

} } } // golos::plugins::account_history

CHAINBASE_SET_INDEX_TYPE(
    golos::plugins::account_history::account_history_object,
    golos::plugins::account_history::account_history_index)


#pragma once

#include <golos/chain/proposal_object.hpp>

#include <golos/protocol/types.hpp>

#include <golos/chain/steem_object_types.hpp>

namespace golos { namespace plugins { namespace database_api {

    struct proposal_api_object final {
        proposal_api_object() = default;

        proposal_api_object(const golos::chain::proposal_object& p);

        protocol::account_name_type author;
        std::string title;
        std::string memo;

        time_point_sec expiration_time;
        optional<time_point_sec> review_period_time;
        std::vector<protocol::operation> proposed_operations;
        flat_set<protocol::account_name_type> required_active_approvals;
        flat_set<protocol::account_name_type> available_active_approvals;
        flat_set<protocol::account_name_type> required_owner_approvals;
        flat_set<protocol::account_name_type> available_owner_approvals;
        flat_set<protocol::account_name_type> required_posting_approvals;
        flat_set<protocol::account_name_type> available_posting_approvals;
        flat_set<protocol::public_key_type> available_key_approvals;
    };

}}} // golos::plugins::database_api

FC_REFLECT(
    (golos::plugins::database_api::proposal_api_object),
    (author)(title)(memo)(expiration_time)(review_period_time)(proposed_operations)
    (required_active_approvals)(available_active_approvals)
    (required_owner_approvals)(available_owner_approvals)
    (required_posting_approvals)(available_posting_approvals)
    (available_key_approvals))
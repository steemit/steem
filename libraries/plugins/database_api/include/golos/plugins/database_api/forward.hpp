#ifndef GOLOS_FORWARD_HPP
#define GOLOS_FORWARD_HPP

#include <golos/chain/steem_objects.hpp>

namespace golos {
    namespace plugins {
        namespace database_api {
            typedef golos::chain::change_recovery_account_request_object change_recovery_account_request_api_object;
            typedef golos::chain::block_summary_object block_summary_api_object;
            typedef golos::chain::comment_vote_object comment_vote_api_object;
            typedef golos::chain::dynamic_global_property_object dynamic_global_property_api_object;
            typedef golos::chain::convert_request_object convert_request_api_object;
            typedef golos::chain::escrow_object escrow_api_object;
            typedef golos::chain::liquidity_reward_balance_object liquidity_reward_balance_api_object;
            typedef golos::chain::limit_order_object limit_order_api_object;
            typedef golos::chain::withdraw_vesting_route_object withdraw_vesting_route_api_object;
            typedef golos::chain::decline_voting_rights_request_object decline_voting_rights_request_api_object;
            typedef golos::chain::witness_vote_object witness_vote_api_object;
            typedef golos::chain::witness_schedule_object witness_schedule_api_object;
            typedef golos::chain::account_bandwidth_object account_bandwidth_api_object;
        }
    }
}
#endif //GOLOS_FORWARD_HPP

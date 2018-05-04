#pragma once

#include <golos/protocol/operations.hpp>
#include <golos/chain/database.hpp>

#include <golos/plugins/mongo_db/mongo_db_types.hpp>


namespace golos {
namespace plugins {
namespace mongo_db {

    class operation_writer {
    public:
        using result_type = document;
        
        operation_writer();

        result_type operator()(const vote_operation& op);
        result_type operator()(const comment_operation& op);
        result_type operator()(const transfer_operation& op);
        result_type operator()(const transfer_to_vesting_operation& op);
        result_type operator()(const withdraw_vesting_operation& op);
        result_type operator()(const limit_order_create_operation& op);
        result_type operator()(const limit_order_cancel_operation& op);
        result_type operator()(const feed_publish_operation& op);
        result_type operator()(const convert_operation& op);
        result_type operator()(const account_create_operation& op);
        result_type operator()(const account_update_operation& op);
        result_type operator()(const witness_update_operation& op);
        result_type operator()(const account_witness_vote_operation& op);
        result_type operator()(const account_witness_proxy_operation& op);
        result_type operator()(const pow_operation& op);
        result_type operator()(const custom_operation& op);
        result_type operator()(const report_over_production_operation& op);
        result_type operator()(const delete_comment_operation& op);
        result_type operator()(const custom_json_operation& op);
        result_type operator()(const comment_options_operation& op);
        result_type operator()(const set_withdraw_vesting_route_operation& op);
        result_type operator()(const limit_order_create2_operation& op);
        result_type operator()(const challenge_authority_operation& op);
        result_type operator()(const prove_authority_operation& op);
        result_type operator()(const request_account_recovery_operation& op);
        result_type operator()(const recover_account_operation& op);
        result_type operator()(const change_recovery_account_operation& op);
        result_type operator()(const escrow_transfer_operation& op);
        result_type operator()(const escrow_dispute_operation& op);
        result_type operator()(const escrow_release_operation&op);
        result_type operator()(const pow2_operation& op);
        result_type operator()(const escrow_approve_operation& op);
        result_type operator()(const transfer_to_savings_operation& op);
        result_type operator()(const transfer_from_savings_operation& op);
        result_type operator()(const cancel_transfer_from_savings_operation&op);
        result_type operator()(const custom_binary_operation& op);
        result_type operator()(const decline_voting_rights_operation& op);
        result_type operator()(const reset_account_operation& op);
        result_type operator()(const set_reset_account_operation& op);
        result_type operator()(const fill_convert_request_operation& op);
        result_type operator()(const author_reward_operation& op);
        result_type operator()(const curation_reward_operation& op);
        result_type operator()(const comment_reward_operation& op);
        result_type operator()(const liquidity_reward_operation& op);
        result_type operator()(const interest_operation& op);
        result_type operator()(const fill_vesting_withdraw_operation& op);
        result_type operator()(const fill_order_operation& op);
        result_type operator()(const shutdown_witness_operation& op);
        result_type operator()(const fill_transfer_from_savings_operation& op);
        result_type operator()(const hardfork_operation& op);
        result_type operator()(const comment_payout_update_operation& op);
        result_type operator()(const comment_benefactor_reward_operation& op);
    };

}}} // golos::plugins::mongo_db

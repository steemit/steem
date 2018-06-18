#pragma once

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/key_extractors.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>

#include <golos/protocol/operations.hpp>
#include <golos/chain/database.hpp>

#include <golos/plugins/mongo_db/mongo_db_types.hpp>

namespace golos {
namespace plugins {
namespace mongo_db {

    class state_writer {
    public:
        using result_type = void;

        state_writer(db_map& bmi_to_add, const signed_block& block);

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
        result_type operator()(const account_create_with_delegation_operation& op);
        result_type operator()(const account_metadata_operation& op);
        result_type operator()(const account_update_operation& op);
        result_type operator()(const witness_update_operation& op);
        result_type operator()(const account_witness_vote_operation& op);
        result_type operator()(const account_witness_proxy_operation& op);
        result_type operator()(const pow_operation& op);
        result_type operator()(const pow2_operation& op);
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
        result_type operator()(const escrow_approve_operation& op);
        result_type operator()(const escrow_dispute_operation& op);
        result_type operator()(const escrow_release_operation&op);
        result_type operator()(const transfer_to_savings_operation& op);
        result_type operator()(const transfer_from_savings_operation& op);
        result_type operator()(const cancel_transfer_from_savings_operation&op);
        result_type operator()(const custom_binary_operation& op);
        result_type operator()(const decline_voting_rights_operation& op);
        result_type operator()(const reset_account_operation& op);
        result_type operator()(const set_reset_account_operation& op);
        result_type operator()(const delegate_vesting_shares_operation& op);
        result_type operator()(const proposal_create_operation& op);
        result_type operator()(const proposal_update_operation& op);
        result_type operator()(const proposal_delete_operation& op);
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
        result_type operator()(const return_vesting_delegation_operation& op);
        result_type operator()(const chain_properties_update_operation& op);

    private:
        database &db_;

        const signed_block &state_block;

        db_map &all_docs;

        bool format_comment(const std::string& auth, const std::string& perm);

        void format_account(const account_object& account);

        void format_account(const std::string& name);

        void format_account_authority(const account_name_type& account_name);

        void format_account_bandwidth(const account_name_type& account, const bandwidth_type& type);

        void format_witness(const witness_object& witness);

        void format_witness(const account_name_type& owner);

        void format_vesting_delegation_object(const vesting_delegation_object& delegation);

        void format_vesting_delegation_object(const account_name_type& delegator,
            const account_name_type& delegatee);

        void format_escrow(const escrow_object &escrow);

        void format_escrow(const account_name_type &name, uint32_t escrow_id);

        void format_global_property_object();

        void format_proposal(const proposal_object& proposal);

        void format_proposal(const account_name_type& author, const std::string& title);

        void format_required_approval(const required_approval_object& reqapp,
            const account_name_type& proposal_author, const std::string& proposal_title);

        named_document create_document(const std::string& name,
            const std::string& key, const std::string& keyval);

        named_document create_removal_document(const std::string& name,
            const std::string& key, const std::string& keyval);
    };

}}} // golos::plugins::mongo_db

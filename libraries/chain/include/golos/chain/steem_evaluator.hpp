#pragma once

#include <golos/protocol/steem_operations.hpp>
#include <golos/protocol/proposal_operations.hpp>
#include <golos/chain/evaluator.hpp>

#define ASSERT_REQ_HF(HF, FEATURE) \
    FC_ASSERT(db().has_hardfork(HF), FEATURE " is not enabled until HF " BOOST_PP_STRINGIZE(HF));

namespace golos { namespace chain {
        using namespace golos::protocol;

        DEFINE_EVALUATOR(account_create)
        DEFINE_EVALUATOR(account_create_with_delegation)
        DEFINE_EVALUATOR(account_update)
        DEFINE_EVALUATOR(account_metadata)
        DEFINE_EVALUATOR(transfer)
        DEFINE_EVALUATOR(transfer_to_vesting)
        DEFINE_EVALUATOR(witness_update)
        DEFINE_EVALUATOR(account_witness_vote)
        DEFINE_EVALUATOR(account_witness_proxy)
        DEFINE_EVALUATOR(withdraw_vesting)
        DEFINE_EVALUATOR(set_withdraw_vesting_route)
        DEFINE_EVALUATOR(comment)
        DEFINE_EVALUATOR(comment_options)
        DEFINE_EVALUATOR(delete_comment)
        DEFINE_EVALUATOR(vote)
        DEFINE_EVALUATOR(custom)
        DEFINE_EVALUATOR(custom_json)
        DEFINE_EVALUATOR(custom_binary)
        DEFINE_EVALUATOR(pow)
        DEFINE_EVALUATOR(pow2)
        DEFINE_EVALUATOR(feed_publish)
        DEFINE_EVALUATOR(convert)
        DEFINE_EVALUATOR(limit_order_create)
        DEFINE_EVALUATOR(limit_order_cancel)
        DEFINE_EVALUATOR(report_over_production)
        DEFINE_EVALUATOR(limit_order_create2)
        DEFINE_EVALUATOR(escrow_transfer)
        DEFINE_EVALUATOR(escrow_approve)
        DEFINE_EVALUATOR(escrow_dispute)
        DEFINE_EVALUATOR(escrow_release)
        DEFINE_EVALUATOR(challenge_authority)
        DEFINE_EVALUATOR(prove_authority)
        DEFINE_EVALUATOR(request_account_recovery)
        DEFINE_EVALUATOR(recover_account)
        DEFINE_EVALUATOR(change_recovery_account)
        DEFINE_EVALUATOR(transfer_to_savings)
        DEFINE_EVALUATOR(transfer_from_savings)
        DEFINE_EVALUATOR(cancel_transfer_from_savings)
        DEFINE_EVALUATOR(decline_voting_rights)
        DEFINE_EVALUATOR(reset_account)
        DEFINE_EVALUATOR(set_reset_account)
        DEFINE_EVALUATOR(delegate_vesting_shares)
        DEFINE_EVALUATOR(proposal_delete)
        DEFINE_EVALUATOR(chain_properties_update)

        class proposal_create_evaluator: public evaluator_impl<proposal_create_evaluator> {
        public:
            using operation_type = proposal_create_operation;

            proposal_create_evaluator(database& db);

            void do_apply(const operation_type& o);

        protected:
            int depth_ = 0;
        };

        class proposal_update_evaluator: public evaluator_impl<proposal_update_evaluator> {
        public:
            using operation_type = proposal_update_operation;

            proposal_update_evaluator(database& db);

            void do_apply(const operation_type& o);

        protected:
            int depth_ = 0;
        };

} } // golos::chain

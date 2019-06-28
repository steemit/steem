#pragma once

#include <dpn/protocol/dpn_operations.hpp>

#include <dpn/chain/evaluator.hpp>

namespace dpn { namespace chain {

using namespace dpn::protocol;

DPN_DEFINE_EVALUATOR( account_create )
DPN_DEFINE_EVALUATOR( account_create_with_delegation )
DPN_DEFINE_EVALUATOR( account_update )
DPN_DEFINE_EVALUATOR( account_update2 )
DPN_DEFINE_EVALUATOR( transfer )
DPN_DEFINE_EVALUATOR( transfer_to_vesting )
DPN_DEFINE_EVALUATOR( witness_update )
DPN_DEFINE_EVALUATOR( account_witness_vote )
DPN_DEFINE_EVALUATOR( account_witness_proxy )
DPN_DEFINE_EVALUATOR( withdraw_vesting )
DPN_DEFINE_EVALUATOR( set_withdraw_vesting_route )
DPN_DEFINE_EVALUATOR( comment )
DPN_DEFINE_EVALUATOR( comment_options )
DPN_DEFINE_EVALUATOR( delete_comment )
DPN_DEFINE_EVALUATOR( vote )
DPN_DEFINE_EVALUATOR( custom )
DPN_DEFINE_EVALUATOR( custom_json )
DPN_DEFINE_EVALUATOR( custom_binary )
DPN_DEFINE_EVALUATOR( pow )
DPN_DEFINE_EVALUATOR( pow2 )
DPN_DEFINE_EVALUATOR( feed_publish )
DPN_DEFINE_EVALUATOR( convert )
DPN_DEFINE_EVALUATOR( limit_order_create )
DPN_DEFINE_EVALUATOR( limit_order_cancel )
DPN_DEFINE_EVALUATOR( report_over_production )
DPN_DEFINE_EVALUATOR( limit_order_create2 )
DPN_DEFINE_EVALUATOR( escrow_transfer )
DPN_DEFINE_EVALUATOR( escrow_approve )
DPN_DEFINE_EVALUATOR( escrow_dispute )
DPN_DEFINE_EVALUATOR( escrow_release )
DPN_DEFINE_EVALUATOR( claim_account )
DPN_DEFINE_EVALUATOR( create_claimed_account )
DPN_DEFINE_EVALUATOR( request_account_recovery )
DPN_DEFINE_EVALUATOR( recover_account )
DPN_DEFINE_EVALUATOR( change_recovery_account )
DPN_DEFINE_EVALUATOR( transfer_to_savings )
DPN_DEFINE_EVALUATOR( transfer_from_savings )
DPN_DEFINE_EVALUATOR( cancel_transfer_from_savings )
DPN_DEFINE_EVALUATOR( decline_voting_rights )
DPN_DEFINE_EVALUATOR( reset_account )
DPN_DEFINE_EVALUATOR( set_reset_account )
DPN_DEFINE_EVALUATOR( claim_reward_balance )
#ifdef DPN_ENABLE_SMT
DPN_DEFINE_EVALUATOR( claim_reward_balance2 )
#endif
DPN_DEFINE_EVALUATOR( delegate_vesting_shares )
DPN_DEFINE_EVALUATOR( witness_set_properties )
#ifdef DPN_ENABLE_SMT
DPN_DEFINE_EVALUATOR( smt_setup )
DPN_DEFINE_EVALUATOR( smt_cap_reveal )
DPN_DEFINE_EVALUATOR( smt_refund )
DPN_DEFINE_EVALUATOR( smt_setup_emissions )
DPN_DEFINE_EVALUATOR( smt_set_setup_parameters )
DPN_DEFINE_EVALUATOR( smt_set_runtime_parameters )
DPN_DEFINE_EVALUATOR( smt_create )
DPN_DEFINE_EVALUATOR( smt_contribute )
#endif
DPN_DEFINE_EVALUATOR( create_proposal )
DPN_DEFINE_EVALUATOR( update_proposal_votes )
DPN_DEFINE_EVALUATOR( remove_proposal )

} } // dpn::chain

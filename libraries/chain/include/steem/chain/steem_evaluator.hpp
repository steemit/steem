#pragma once

#include <steem/protocol/steem_operations.hpp>

#include <steem/chain/evaluator.hpp>

namespace steem { namespace chain {

using namespace steem::protocol;

STEEM_DEFINE_EVALUATOR( account_create )
STEEM_DEFINE_EVALUATOR( account_create_with_delegation )
STEEM_DEFINE_EVALUATOR( account_update )
STEEM_DEFINE_EVALUATOR( transfer )
STEEM_DEFINE_EVALUATOR( transfer_to_vesting )
STEEM_DEFINE_EVALUATOR( witness_update )
STEEM_DEFINE_EVALUATOR( account_witness_vote )
STEEM_DEFINE_EVALUATOR( account_witness_proxy )
STEEM_DEFINE_EVALUATOR( withdraw_vesting )
STEEM_DEFINE_EVALUATOR( set_withdraw_vesting_route )
STEEM_DEFINE_EVALUATOR( comment )
STEEM_DEFINE_EVALUATOR( comment_options )
STEEM_DEFINE_EVALUATOR( delete_comment )
STEEM_DEFINE_EVALUATOR( vote )
STEEM_DEFINE_EVALUATOR( custom )
STEEM_DEFINE_EVALUATOR( custom_json )
STEEM_DEFINE_EVALUATOR( custom_binary )
STEEM_DEFINE_EVALUATOR( pow )
STEEM_DEFINE_EVALUATOR( pow2 )
STEEM_DEFINE_EVALUATOR( feed_publish )
STEEM_DEFINE_EVALUATOR( convert )
STEEM_DEFINE_EVALUATOR( limit_order_create )
STEEM_DEFINE_EVALUATOR( limit_order_cancel )
STEEM_DEFINE_EVALUATOR( report_over_production )
STEEM_DEFINE_EVALUATOR( limit_order_create2 )
STEEM_DEFINE_EVALUATOR( escrow_transfer )
STEEM_DEFINE_EVALUATOR( escrow_approve )
STEEM_DEFINE_EVALUATOR( escrow_dispute )
STEEM_DEFINE_EVALUATOR( escrow_release )
STEEM_DEFINE_EVALUATOR( claim_account )
STEEM_DEFINE_EVALUATOR( create_claimed_account )
STEEM_DEFINE_EVALUATOR( request_account_recovery )
STEEM_DEFINE_EVALUATOR( recover_account )
STEEM_DEFINE_EVALUATOR( change_recovery_account )
STEEM_DEFINE_EVALUATOR( transfer_to_savings )
STEEM_DEFINE_EVALUATOR( transfer_from_savings )
STEEM_DEFINE_EVALUATOR( cancel_transfer_from_savings )
STEEM_DEFINE_EVALUATOR( decline_voting_rights )
STEEM_DEFINE_EVALUATOR( reset_account )
STEEM_DEFINE_EVALUATOR( set_reset_account )
STEEM_DEFINE_EVALUATOR( claim_reward_balance )
#ifdef STEEM_ENABLE_SMT
STEEM_DEFINE_EVALUATOR( claim_reward_balance2 )
#endif
STEEM_DEFINE_EVALUATOR( delegate_vesting_shares )
STEEM_DEFINE_EVALUATOR( witness_set_properties )
#ifdef STEEM_ENABLE_SMT
STEEM_DEFINE_EVALUATOR( smt_setup )
STEEM_DEFINE_EVALUATOR( smt_cap_reveal )
STEEM_DEFINE_EVALUATOR( smt_refund )
STEEM_DEFINE_EVALUATOR( smt_setup_emissions )
STEEM_DEFINE_EVALUATOR( smt_set_setup_parameters )
STEEM_DEFINE_EVALUATOR( smt_set_runtime_parameters )
STEEM_DEFINE_EVALUATOR( smt_create )
#endif

} } // steem::chain

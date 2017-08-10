#pragma once

#include <steemit/protocol/steem_operations.hpp>

#include <steemit/chain/evaluator.hpp>

namespace steemit{ namespace chain {

using namespace steemit::protocol;

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
STEEM_DEFINE_EVALUATOR( challenge_authority )
STEEM_DEFINE_EVALUATOR( prove_authority )
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
STEEM_DEFINE_EVALUATOR( delegate_vesting_shares )

} } // steemit::chain

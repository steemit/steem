
#include <steem/chain/steem_object_types.hpp>

#include <steem/chain/index.hpp>

#include <steem/chain/block_summary_object.hpp>
#include <steem/chain/history_object.hpp>
#include <steem/chain/pending_required_action_object.hpp>
#include <steem/chain/pending_optional_action_object.hpp>
#include <steem/chain/smt_objects.hpp>
#include <steem/chain/steem_objects.hpp>
#include <steem/chain/sps_objects.hpp>
#include <steem/chain/transaction_object.hpp>
#include <steem/chain/witness_schedule.hpp>

namespace steem { namespace chain {

void initialize_core_indexes( database& db )
{
   STEEM_ADD_CORE_INDEX(db, dynamic_global_property_index);
   STEEM_ADD_CORE_INDEX(db, account_index);
   STEEM_ADD_CORE_INDEX(db, account_metadata_index);
   STEEM_ADD_CORE_INDEX(db, account_authority_index);
   STEEM_ADD_CORE_INDEX(db, witness_index);
   STEEM_ADD_CORE_INDEX(db, transaction_index);
   STEEM_ADD_CORE_INDEX(db, block_summary_index);
   STEEM_ADD_CORE_INDEX(db, witness_schedule_index);
   STEEM_ADD_CORE_INDEX(db, comment_index);
   STEEM_ADD_CORE_INDEX(db, comment_content_index);
   STEEM_ADD_CORE_INDEX(db, comment_vote_index);
   STEEM_ADD_CORE_INDEX(db, witness_vote_index);
   STEEM_ADD_CORE_INDEX(db, limit_order_index);
   STEEM_ADD_CORE_INDEX(db, feed_history_index);
   STEEM_ADD_CORE_INDEX(db, convert_request_index);
   STEEM_ADD_CORE_INDEX(db, liquidity_reward_balance_index);
   STEEM_ADD_CORE_INDEX(db, operation_index);
   STEEM_ADD_CORE_INDEX(db, account_history_index);
   STEEM_ADD_CORE_INDEX(db, hardfork_property_index);
   STEEM_ADD_CORE_INDEX(db, withdraw_vesting_route_index);
   STEEM_ADD_CORE_INDEX(db, owner_authority_history_index);
   STEEM_ADD_CORE_INDEX(db, account_recovery_request_index);
   STEEM_ADD_CORE_INDEX(db, change_recovery_account_request_index);
   STEEM_ADD_CORE_INDEX(db, escrow_index);
   STEEM_ADD_CORE_INDEX(db, savings_withdraw_index);
   STEEM_ADD_CORE_INDEX(db, decline_voting_rights_request_index);
   STEEM_ADD_CORE_INDEX(db, reward_fund_index);
   STEEM_ADD_CORE_INDEX(db, vesting_delegation_index);
   STEEM_ADD_CORE_INDEX(db, vesting_delegation_expiration_index);
   STEEM_ADD_CORE_INDEX(db, pending_required_action_index);
   STEEM_ADD_CORE_INDEX(db, pending_optional_action_index);
   STEEM_ADD_CORE_INDEX(db, smt_token_index);
   STEEM_ADD_CORE_INDEX(db, account_regular_balance_index);
   STEEM_ADD_CORE_INDEX(db, account_rewards_balance_index);
   STEEM_ADD_CORE_INDEX(db, nai_pool_index);
   STEEM_ADD_CORE_INDEX(db, smt_token_emissions_index);
   STEEM_ADD_CORE_INDEX(db, smt_contribution_index);
   STEEM_ADD_CORE_INDEX(db, smt_ico_index);
   STEEM_ADD_CORE_INDEX(db, comment_smt_beneficiaries_index);
   STEEM_ADD_CORE_INDEX(db, proposal_index);
   STEEM_ADD_CORE_INDEX(db, proposal_vote_index);
}

index_info::index_info() {}
index_info::~index_info() {}

} }

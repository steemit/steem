
#include <dpn/chain/dpn_object_types.hpp>

#include <dpn/chain/index.hpp>

#include <dpn/chain/block_summary_object.hpp>
#include <dpn/chain/history_object.hpp>
#include <dpn/chain/pending_required_action_object.hpp>
#include <dpn/chain/pending_optional_action_object.hpp>
#include <dpn/chain/smt_objects.hpp>
#include <dpn/chain/dpn_objects.hpp>
#include <dpn/chain/sps_objects.hpp>
#include <dpn/chain/transaction_object.hpp>
#include <dpn/chain/witness_schedule.hpp>

namespace dpn { namespace chain {

void initialize_core_indexes( database& db )
{
   DPN_ADD_CORE_INDEX(db, dynamic_global_property_index);
   DPN_ADD_CORE_INDEX(db, account_index);
   DPN_ADD_CORE_INDEX(db, account_metadata_index);
   DPN_ADD_CORE_INDEX(db, account_authority_index);
   DPN_ADD_CORE_INDEX(db, witness_index);
   DPN_ADD_CORE_INDEX(db, transaction_index);
   DPN_ADD_CORE_INDEX(db, block_summary_index);
   DPN_ADD_CORE_INDEX(db, witness_schedule_index);
   DPN_ADD_CORE_INDEX(db, comment_index);
   DPN_ADD_CORE_INDEX(db, comment_content_index);
   DPN_ADD_CORE_INDEX(db, comment_vote_index);
   DPN_ADD_CORE_INDEX(db, witness_vote_index);
   DPN_ADD_CORE_INDEX(db, limit_order_index);
   DPN_ADD_CORE_INDEX(db, feed_history_index);
   DPN_ADD_CORE_INDEX(db, convert_request_index);
   DPN_ADD_CORE_INDEX(db, liquidity_reward_balance_index);
   DPN_ADD_CORE_INDEX(db, operation_index);
   DPN_ADD_CORE_INDEX(db, account_history_index);
   DPN_ADD_CORE_INDEX(db, hardfork_property_index);
   DPN_ADD_CORE_INDEX(db, withdraw_vesting_route_index);
   DPN_ADD_CORE_INDEX(db, owner_authority_history_index);
   DPN_ADD_CORE_INDEX(db, account_recovery_request_index);
   DPN_ADD_CORE_INDEX(db, change_recovery_account_request_index);
   DPN_ADD_CORE_INDEX(db, escrow_index);
   DPN_ADD_CORE_INDEX(db, savings_withdraw_index);
   DPN_ADD_CORE_INDEX(db, decline_voting_rights_request_index);
   DPN_ADD_CORE_INDEX(db, reward_fund_index);
   DPN_ADD_CORE_INDEX(db, vesting_delegation_index);
   DPN_ADD_CORE_INDEX(db, vesting_delegation_expiration_index);
   DPN_ADD_CORE_INDEX(db, pending_required_action_index);
   DPN_ADD_CORE_INDEX(db, pending_optional_action_index);
#ifdef DPN_ENABLE_SMT
   DPN_ADD_CORE_INDEX(db, smt_token_index);
   DPN_ADD_CORE_INDEX(db, account_regular_balance_index);
   DPN_ADD_CORE_INDEX(db, account_rewards_balance_index);
   DPN_ADD_CORE_INDEX(db, nai_pool_index);
   DPN_ADD_CORE_INDEX(db, smt_token_emissions_index);
   DPN_ADD_CORE_INDEX(db, smt_contribution_index);
   DPN_ADD_CORE_INDEX(db, smt_ico_index);
#endif
   DPN_ADD_CORE_INDEX(db, proposal_index);
   DPN_ADD_CORE_INDEX(db, proposal_vote_index);
}

index_info::index_info() {}
index_info::~index_info() {}

} }

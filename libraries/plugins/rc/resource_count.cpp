
#include <steem/plugins/rc/resource_count.hpp>
#include <steem/plugins/rc/resource_sizes.hpp>

#include <steem/protocol/operations.hpp>

namespace steem { namespace plugins { namespace rc {

using namespace steem::protocol;

struct count_operation_visitor
{
   typedef void result_type;

   mutable int32_t market_op_count = 0;
   mutable int32_t new_account_op_count = 0;
   mutable int64_t state_bytes_count = 0;
   mutable int64_t execution_time_count = 0;

   const state_object_size_info& _w;
   const operation_exec_info& _e;

   count_operation_visitor( const state_object_size_info& w, const operation_exec_info& e ) : _w(w), _e(e) {}

   int64_t get_authority_byte_count( const authority& auth )const
   {
      return _w.authority_base_size
           + _w.authority_account_member_size * auth.account_auths.size()
           + _w.authority_key_member_size * auth.key_auths.size();
   }

   void operator()( const account_create_operation& op )const
   {
      state_bytes_count +=
           _w.account_object_base_size
         + _w.account_authority_object_base_size
         + get_authority_byte_count( op.owner )
         + get_authority_byte_count( op.active )
         + get_authority_byte_count( op.posting );
      execution_time_count += _e.account_create_operation_exec_time;
   }

   void operator()( const account_create_with_delegation_operation& op )const
   {
      state_bytes_count +=
           _w.account_object_base_size
         + _w.account_authority_object_base_size
         + get_authority_byte_count( op.owner )
         + get_authority_byte_count( op.active )
         + get_authority_byte_count( op.posting )
         + _w.vesting_delegation_object_base_size;
      execution_time_count += _e.account_create_with_delegation_operation_exec_time;
   }

   void operator()( const account_witness_vote_operation& op )const
   {
      state_bytes_count += _w.witness_vote_object_base_size;
      execution_time_count += _e.account_witness_vote_operation_exec_time;
   }

   void operator()( const comment_operation& op )const
   {
      state_bytes_count +=
           _w.comment_object_base_size
         + _w.comment_object_permlink_char_size * op.permlink.size()
         + _w.comment_object_parent_permlink_char_size * op.parent_permlink.size();
      execution_time_count += _e.comment_operation_exec_time;
   }

   void operator()( const comment_payout_beneficiaries& bens )const
   {
      state_bytes_count += _w.comment_object_beneficiaries_member_size * bens.beneficiaries.size();
   }

   void operator()( const comment_options_operation& op )const
   {
      for( const comment_options_extension& e : op.extensions )
      {
         e.visit( *this );
      }
      execution_time_count += _e.comment_options_operation_exec_time;
   }

   void operator()( const convert_operation& op ) const
   {
      state_bytes_count += _w.convert_request_object_base_size;
      execution_time_count += _e.convert_operation_exec_time;
   }

   void operator()( const create_claimed_account_operation& op )const
   {
      state_bytes_count +=
           _w.account_object_base_size
         + _w.account_authority_object_base_size
         + get_authority_byte_count( op.owner )
         + get_authority_byte_count( op.active )
         + get_authority_byte_count( op.posting );
      execution_time_count += _e.create_claimed_account_operation_exec_time;
   }

   void operator()( const decline_voting_rights_operation& op )const
   {
      state_bytes_count += _w.decline_voting_rights_request_object_base_size;
      execution_time_count += _e.decline_voting_rights_operation_exec_time;
   }

   void operator()( const delegate_vesting_shares_operation& op )const
   {
      state_bytes_count += std::max(
         _w.vesting_delegation_object_base_size,
         _w.vesting_delegation_expiration_object_base_size
         );
      execution_time_count += _e.delegate_vesting_shares_operation_exec_time;
   }

   void operator()( const escrow_transfer_operation& op )const
   {
      state_bytes_count += _w.escrow_object_base_size;
      execution_time_count += _e.escrow_transfer_operation_exec_time;
   }

   void operator()( const limit_order_create_operation& op )const
   {
      state_bytes_count += op.fill_or_kill ? 0 : _w.limit_order_object_base_size;
      execution_time_count += _e.limit_order_create_operation_exec_time;
      market_op_count++;
   }

   void operator()( const limit_order_create2_operation& op )const
   {
      state_bytes_count += op.fill_or_kill ? 0 : _w.limit_order_object_base_size;
      execution_time_count += _e.limit_order_create2_operation_exec_time;
      market_op_count++;
   }

   void operator()( const request_account_recovery_operation& op )const
   {
      state_bytes_count += _w.account_recovery_request_object_base_size;
      execution_time_count += _e.request_account_recovery_operation_exec_time;
   }

   void operator()( const set_withdraw_vesting_route_operation& op )const
   {
      FC_TODO( "Change RC state bytes computation to take SMT's into account" )
      state_bytes_count += _w.withdraw_vesting_route_object_base_size;
      execution_time_count += _e.set_withdraw_vesting_route_operation_exec_time;
   }

   void operator()( const vote_operation& op )const
   {
      FC_TODO( "Change RC state bytes computation to take SMT's into account" )
      state_bytes_count += _w.comment_vote_object_base_size;
      execution_time_count += _e.vote_operation_exec_time;
   }

   void operator()( const witness_update_operation& op )const
   {
      state_bytes_count +=
           _w.witness_object_base_size
         + _w.witness_object_url_char_size * op.url.size();
      execution_time_count += _e.witness_update_operation_exec_time;
   }

   void operator()( const transfer_operation& )const
   {
      FC_TODO( "Change RC state bytes computation to take SMT's into account" )
      execution_time_count += _e.transfer_operation_exec_time;
      market_op_count++;
   }

   void operator()( const transfer_to_vesting_operation& )const
   {
      FC_TODO( "Change RC state bytes computation to take SMT's into account" )
      execution_time_count += _e.transfer_to_vesting_operation_exec_time;
      market_op_count++;
   }

   void operator()( const transfer_to_savings_operation& )const
   {
      FC_TODO( "Change RC state bytes computation to take SMT's into account" )
      execution_time_count += _e.transfer_to_savings_operation_exec_time;
   }

   void operator()( const transfer_from_savings_operation& )const
   {
      FC_TODO( "Change RC state bytes computation to take SMT's into account" )
      state_bytes_count += _w.savings_withdraw_object_byte_size;
      execution_time_count += _e.transfer_from_savings_operation_exec_time;
   }

   void operator()( const claim_reward_balance_operation& op )const
   {
      FC_TODO( "Change RC state bytes computation to take SMT's into account" )
      execution_time_count += _e.claim_reward_balance_operation_exec_time;
   }

   void operator()( const withdraw_vesting_operation& op )const
   {
      FC_TODO( "Change RC state bytes computation to take SMT's into account" )
      execution_time_count += _e.withdraw_vesting_operation_exec_time;
   }

   void operator()( const account_update_operation& )const
   {
      execution_time_count += _e.account_update_operation_exec_time;
   }

   void operator()( const account_update2_operation& )const
   {
      execution_time_count += _e.account_update2_operation_exec_time;
   }

   void operator()( const account_witness_proxy_operation& )const
   {
      execution_time_count += _e.account_witness_proxy_operation_exec_time;
   }

   void operator()( const cancel_transfer_from_savings_operation& )const
   {
      execution_time_count += _e.cancel_transfer_from_savings_operation_exec_time;
   }

   void operator()( const change_recovery_account_operation& )const
   {
      execution_time_count += _e.change_recovery_account_operation_exec_time;
   }

   void operator()( const claim_account_operation& o )const
   {
      execution_time_count += _e.claim_account_operation_exec_time;

      if( o.fee.amount == 0 )
      {
         new_account_op_count++;
      }
   }

   void operator()( const custom_operation& )const
   {
      execution_time_count += _e.custom_operation_exec_time;
   }

   void operator()( const custom_json_operation& o )const
   {
      auto exec_time = _e.custom_operation_exec_time;

      if( o.id == "follow" )
      {
         exec_time *= EXEC_FOLLOW_CUSTOM_OP_SCALE;
      }

      execution_time_count += exec_time;
   }

   void operator()( const custom_binary_operation& o )const
   {
      auto exec_time = _e.custom_operation_exec_time;

      if( o.id == "follow" )
      {
         exec_time *= EXEC_FOLLOW_CUSTOM_OP_SCALE;
      }

      execution_time_count += exec_time;
   }

   void operator()( const delete_comment_operation& )const
   {
      execution_time_count += _e.delete_comment_operation_exec_time;
   }

   void operator()( const escrow_approve_operation& )const
   {
      execution_time_count += _e.escrow_approve_operation_exec_time;
   }

   void operator()( const escrow_dispute_operation& )const
   {
      execution_time_count += _e.escrow_dispute_operation_exec_time;
   }

   void operator()( const escrow_release_operation& )const
   {
      execution_time_count += _e.escrow_release_operation_exec_time;
   }

   void operator()( const feed_publish_operation& )const
   {
      execution_time_count += _e.feed_publish_operation_exec_time;
   }

   void operator()( const limit_order_cancel_operation& )const
   {
      execution_time_count += _e.limit_order_cancel_operation_exec_time;
   }

   void operator()( const witness_set_properties_operation& )const
   {
      execution_time_count += _e.witness_set_properties_operation_exec_time;
   }

#ifdef STEEM_ENABLE_SMT
   void operator()( const claim_reward_balance2_operation& op )const
   {
      FC_TODO( "Change RC state bytes computation to take SMT's into account" )
      execution_time_count += _e.claim_reward_balance2_operation_exec_time;
   }

   void operator()( const smt_setup_operation& op )const
   {
      FC_TODO( "Change RC state bytes computation to take SMT's into account" )
      execution_time_count += _e.smt_setup_operation_exec_time;
   }

   void operator()( const smt_setup_emissions_operation& op )const
   {
      FC_TODO( "Change RC state bytes computation to take SMT's into account" )
      execution_time_count += _e.smt_setup_emissions_operation_exec_time;
   }

   void operator()( const smt_set_setup_parameters_operation& op )const
   {
      FC_TODO( "Change RC state bytes computation to take SMT's into account" )
      execution_time_count += _e.smt_set_setup_parameters_operation_exec_time;
   }

   void operator()( const smt_set_runtime_parameters_operation& op )const
   {
      FC_TODO( "Change RC state bytes computation to take SMT's into account" )
      execution_time_count += _e.smt_set_runtime_parameters_operation_exec_time;
   }

   void operator()( const smt_create_operation& op )const
   {
      FC_TODO( "Change RC state bytes computation to take SMT's into account" )
      execution_time_count += _e.smt_create_operation_exec_time;
   }

   void operator()( const allowed_vote_assets& )const
   {
      FC_TODO( "Change RC state bytes computation to take SMT's into account" )
   }

   void operator()( const smt_contribute_operation& op ) const
   {
      FC_TODO( "Change RC state bytes computation to take SMT's into account" )
      execution_time_count += _e.smt_contribute_operation_exec_time;
   }
#endif

   void operator()( const create_proposal_operation& op ) const
   {
      state_bytes_count += _w.proposal_object_base_size;
      state_bytes_count += sizeof( op.subject );
      state_bytes_count += sizeof( op.permlink );
      execution_time_count += _e.create_proposal_operation_exec_time;
   }

   void operator()( const update_proposal_votes_operation& op ) const
   {
      state_bytes_count += _w.proposal_vote_object_base_size;
      state_bytes_count += _w.proposal_vote_object_member_size * op.proposal_ids.size();
      execution_time_count += _e.update_proposal_votes_operation_exec_time;
   }

   void operator()(const remove_proposal_operation&) const
   {
      execution_time_count += _e.remove_proposal_operation_exec_time;
   }

   void operator()( const recover_account_operation& ) const {}
   void operator()( const pow_operation& ) const {}
   void operator()( const pow2_operation& ) const {}
   void operator()( const report_over_production_operation& ) const {}
   void operator()( const reset_account_operation& ) const {}
   void operator()( const set_reset_account_operation& ) const {}

   // Virtual Ops
   void operator()( const fill_convert_request_operation& ) const {}
   void operator()( const author_reward_operation& ) const {}
   void operator()( const curation_reward_operation& ) const {}
   void operator()( const comment_reward_operation& ) const {}
   void operator()( const liquidity_reward_operation& ) const {}
   void operator()( const interest_operation& ) const {}
   void operator()( const fill_vesting_withdraw_operation& ) const {}
   void operator()( const fill_order_operation& ) const {}
   void operator()( const shutdown_witness_operation& ) const {}
   void operator()( const fill_transfer_from_savings_operation& ) const {}
   void operator()( const hardfork_operation& ) const {}
   void operator()( const comment_payout_update_operation& ) const {}
   void operator()( const return_vesting_delegation_operation& ) const {}
   void operator()( const comment_benefactor_reward_operation& ) const {}
   void operator()( const producer_reward_operation& ) const {}
   void operator()( const clear_null_account_balance_operation& ) const {}
   void operator()( const proposal_pay_operation& ) const {}
   void operator()( const sps_fund_operation& ) const {}

   // Optional Actions
#ifdef IS_TEST_NET
   void operator()( const example_optional_action& ) const {}
#endif


   // TODO:
   // Should following ops be market ops?
   // withdraw_vesting, convert, set_withdraw_vesting_route, limit_order_create2
   // escrow_transfer, escrow_dispute, escrow_release, escrow_approve,
   // transfer_to_savings, transfer_from_savings, cancel_transfer_from_savings,
   // claim_reward_balance, delegate_vesting_shares, any SMT operations
};

typedef count_operation_visitor count_optional_action_visitor;

void count_resources(
   const signed_transaction& tx,
   count_resources_result& result )
{
   static const state_object_size_info size_info;
   static const operation_exec_info exec_info;
   const int64_t tx_size = int64_t( fc::raw::pack_size( tx ) );
   count_operation_visitor vtor( size_info, exec_info );

   result.resource_count[ resource_history_bytes ] += tx_size;

   for( const operation& op : tx.operations )
   {
      op.visit( vtor );
   }

   result.resource_count[ resource_new_accounts ] += vtor.new_account_op_count;

   if( vtor.market_op_count > 0 )
      result.resource_count[ resource_market_bytes ] += tx_size;

   result.resource_count[ resource_state_bytes ] +=
        size_info.transaction_object_base_size
      + size_info.transaction_object_byte_size * tx_size
      + vtor.state_bytes_count;

   result.resource_count[ resource_execution_time ] += vtor.execution_time_count;
}

void count_resources(
   const optional_automated_action& action,
   count_resources_result& result )
{
   static const state_object_size_info size_info;
   static const operation_exec_info exec_info;
   const int64_t action_size = int64_t( fc::raw::pack_size( action ) );
   count_optional_action_visitor vtor( size_info, exec_info );

   result.resource_count[ resource_history_bytes ] += action_size;

   action.visit( vtor );

   // Not expecting actions to create accounts, but better to add it for completeness
   result.resource_count[ resource_new_accounts ] += vtor.new_account_op_count;

   result.resource_count[ resource_state_bytes ] += vtor.state_bytes_count;

   result.resource_count[ resource_execution_time ] += vtor.execution_time_count;
}

} } } // steem::plugins::rc

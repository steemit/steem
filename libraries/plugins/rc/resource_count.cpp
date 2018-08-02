
#include <steem/plugins/rc/resource_count.hpp>

#include <steem/protocol/operations.hpp>
#include <steem/protocol/transaction.hpp>

#define STATE_BYTES_SCALE                       10000

// These numbers are obtained from computations in state_byte_hours.py script
//
// $ ./state_byte_hours.py
// {"seconds":196608}
// 174
// {"days":3}
// 229
// {"days":28}
// 1940

#define STATE_TRANSACTION_BYTE_SIZE               174
#define STATE_TRANSFER_FROM_SAVINGS_BYTE_SIZE     229
#define STATE_LIMIT_ORDER_BYTE_SIZE              1940

#define RC_DEFAULT_EXEC_COST 100000

namespace steem { namespace plugins { namespace rc {

using namespace steem::protocol;

struct state_object_size_info
{
   // authority
   int64_t authority_base_size                =   4    *STATE_BYTES_SCALE;
   int64_t authority_account_member_size      =  18    *STATE_BYTES_SCALE;
   int64_t authority_key_member_size          =  35    *STATE_BYTES_SCALE;

   // account_object
   int64_t account_object_base_size           = 480    *STATE_BYTES_SCALE;
   int64_t account_authority_object_base_size =  40    *STATE_BYTES_SCALE;

   // account_recovery_request_object
   int64_t account_recovery_request_object_base_size = 32*STATE_BYTES_SCALE;

   // comment_object
   int64_t comment_object_base_size           = 201    *STATE_BYTES_SCALE;
   // category isn't actually a field in the op, it's done by parent_permlink
   // int64_t comment_object_category_char_size  =         STATE_BYTES_SCALE;
   int64_t comment_object_permlink_char_size  =         STATE_BYTES_SCALE;
   int64_t comment_object_parent_permlink_char_size = 2*STATE_BYTES_SCALE;
   int64_t comment_object_beneficiaries_member_size = 18*STATE_BYTES_SCALE;

   // comment_vote_object
   int64_t comment_vote_object_base_size      = 47     *STATE_BYTES_SCALE;

   // convert_request_object
   int64_t convert_request_object_base_size   = 48     *STATE_BYTES_SCALE;

   // decline_voting_rights_request_object
   int64_t decline_voting_rights_request_object_base_size = 28*STATE_BYTES_SCALE;

   // escrow_object
   int64_t escrow_object_base_size            = 119    *STATE_BYTES_SCALE;

   // limit_order_object
   int64_t limit_order_object_base_size       = 76     *STATE_LIMIT_ORDER_BYTE_SIZE;

   // savigns_withdraw_object
   int64_t savings_withdraw_object_byte_size  = 64     *STATE_TRANSFER_FROM_SAVINGS_BYTE_SIZE;

   // transaction_object
   int64_t transaction_object_base_size       =  35    *STATE_TRANSACTION_BYTE_SIZE;
   int64_t transaction_object_byte_size       =         STATE_TRANSACTION_BYTE_SIZE;

   // vesting_delegation_object
   int64_t vesting_delegation_object_base_size = 60*STATE_BYTES_SCALE;

   // vesting_delegation_expiration_object
   int64_t vesting_delegation_expiration_object_base_size = 44*STATE_BYTES_SCALE;

   // withdraw_vesting_route_object
   int64_t withdraw_vesting_route_object_base_size = 43*STATE_BYTES_SCALE;

   // witness_object
   int64_t witness_object_base_size           = 266    *STATE_BYTES_SCALE;
   int64_t witness_object_url_char_size       =   1    *STATE_BYTES_SCALE;

   // witness_vote_object
   int64_t witness_vote_object_base_size      = 40     *STATE_BYTES_SCALE;
};

struct operation_exec_info
{
   int64_t account_create_operation_exec_time                  =  57700;
   int64_t account_create_with_delegation_operation_exec_time  =  57700;
   int64_t account_update_operation_exec_time                  =  14000;
   int64_t account_witness_proxy_operation_exec_time           = 117000;
   int64_t account_witness_vote_operation_exec_time            =  23000;
   int64_t cancel_transfer_from_savings_operation_exec_time    =  11500;
   int64_t change_recovery_account_operation_exec_time         =  12000;
   int64_t claim_account_operation_exec_time                   =  10000;
   int64_t claim_reward_balance_operation_exec_time            =  50300;
   int64_t comment_operation_exec_time                         = 114100;
   int64_t comment_options_operation_exec_time                 =  13200;
   int64_t convert_operation_exec_time                         =  15700;
   int64_t create_claimed_account_operation_exec_time          =  57700;
   int64_t custom_operation_exec_time                          = 228000;
   int64_t custom_json_operation_exec_time                     = 228000;
   int64_t custom_binary_operation_exec_time                   = 228000;
   int64_t decline_voting_rights_operation_exec_time           =   5300;
   int64_t delegate_vesting_shares_operation_exec_time         =  19900;
   int64_t delete_comment_operation_exec_time                  =  51100;
   int64_t escrow_approve_operation_exec_time                  =   9900;
   int64_t escrow_dispute_operation_exec_time                  =  11500;
   int64_t escrow_release_operation_exec_time                  =  17200;
   int64_t escrow_transfer_operation_exec_time                 =  19100;
   int64_t feed_publish_operation_exec_time                    =   6200;
   int64_t limit_order_cancel_operation_exec_time              =   9600;
   int64_t limit_order_create_operation_exec_time              =  31700;
   int64_t limit_order_create2_operation_exec_time             =  31700;
   int64_t request_account_recovery_operation_exec_time        =  54400;
   int64_t set_withdraw_vesting_route_operation_exec_time      =  17900;
   int64_t transfer_from_savings_operation_exec_time           =  17500;
   int64_t transfer_operation_exec_time                        =   9600;
   int64_t transfer_to_savings_operation_exec_time             =   6400;
   int64_t transfer_to_vesting_operation_exec_time             =  44400;
   int64_t vote_operation_exec_time                            =  26500;
   int64_t withdraw_vesting_operation_exec_time                =  10400;
   int64_t witness_set_properties_operation_exec_time          =   9500;
   int64_t witness_update_operation_exec_time                  =   9500;

#ifdef STEEM_ENABLE_SMT
   int64_t claim_reward_balance2_operation_exec_time           = 0;
   int64_t smt_setup_operation_exec_time                       = 0;
   int64_t smt_cap_reveal_operation_exec_time                  = 0;
   int64_t smt_refund_operation_exec_time                      = 0;
   int64_t smt_setup_emissions_operation_exec_time             = 0;
   int64_t smt_set_setup_parameters_operation_exec_time        = 0;
   int64_t smt_set_runtime_parameters_operation_exec_time      = 0;
   int64_t smt_create_operation_exec_time                      = 0;
#endif
};

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
      new_account_op_count++;
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
      new_account_op_count++;
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

   void operator()( const claim_account_operation& )const
   {
      execution_time_count += _e.claim_account_operation_exec_time;
   }

   void operator()( const custom_operation& )const
   {
      execution_time_count += _e.custom_operation_exec_time;
   }

   void operator()( const custom_json_operation& )const
   {
      execution_time_count += _e.custom_json_operation_exec_time;
   }

   void operator()( const custom_binary_operation& )const
   {
      execution_time_count += _e.custom_binary_operation_exec_time;
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
   void operator()()

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

   void operator()( const smt_cap_reveal_operation& op )const
   {
      FC_TODO( "Change RC state bytes computation to take SMT's into account" )
      execution_time_count += _e.smt_cap_reveal_operation_exec_time;
   }

   void operator()( const smt_refund_operation& op )const
   {
      FC_TODO( "Change RC state bytes computation to take SMT's into account" )
      execution_time_count += _e.smt_refund_operation_exec_time;
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
#endif

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


   // TODO:
   // Should following ops be market ops?
   // withdraw_vesting, convert, set_withdraw_vesting_route, limit_order_create2
   // escrow_transfer, escrow_dispute, escrow_release, escrow_approve,
   // transfer_to_savings, transfer_from_savings, cancel_transfer_from_savings,
   // claim_reward_balance, delegate_vesting_shares, any SMT operations
};

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
}

} } } // steem::plugins::rc


#include <steem/plugins/rc/resource_count.hpp>

#include <steem/protocol/transaction.hpp>

#define STATE_BYTES_SCALE              10000
#define STATE_TRANSACTION_BYTE_SIZE      175

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
   int64_t limit_order_object_base_size       = 76     *STATE_BYTES_SCALE;

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

struct count_operation_visitor
{
   typedef void result_type;

   mutable int32_t market_op_count = 0;
   mutable int32_t new_account_op_count = 0;
   mutable int64_t state_bytes_count = 0;

   const state_object_size_info& _w;

   count_operation_visitor( const state_object_size_info& w ) : _w(w) {}

   int64_t get_authority_byte_count( const authority& auth )const
   {
      return _w.authority_base_size
           + _w.authority_account_member_size * auth.account_auths.size()
           + _w.authority_key_member_size * auth.key_auths.size();
   }

   template< typename OpType >
   void operator()( const OpType op )const {}

   void operator()( const account_create_operation& op )const
   {
      state_bytes_count +=
           _w.account_object_base_size
         + _w.account_authority_object_base_size
         + get_authority_byte_count( op.owner )
         + get_authority_byte_count( op.active )
         + get_authority_byte_count( op.posting );
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
      new_account_op_count++;
   }

   void operator()( const account_witness_vote_operation& op )const
   { state_bytes_count += _w.witness_vote_object_base_size;       }

   void operator()( const comment_operation& op )const
   {
      state_bytes_count +=
           _w.comment_object_base_size
         + _w.comment_object_permlink_char_size * op.permlink.size()
         + _w.comment_object_parent_permlink_char_size * op.parent_permlink.size()
         ;
   }

   void operator()( const comment_payout_beneficiaries& bens )const
   {
      state_bytes_count += _w.comment_object_beneficiaries_member_size * bens.beneficiaries.size();
   }

   void operator()( const comment_options_operation& op )const
   {
      for( const comment_options_extension& e : op.extensions )
         e.visit( *this );
   }

   void operator()( const convert_operation& op ) const
   { state_bytes_count += _w.convert_request_object_base_size;    }

   void operator()( const create_claimed_account_operation& op )const
   {
      state_bytes_count +=
           _w.account_object_base_size
         + _w.account_authority_object_base_size
         + get_authority_byte_count( op.owner )
         + get_authority_byte_count( op.active )
         + get_authority_byte_count( op.posting );
   }

   void operator()( const decline_voting_rights_operation& op )const
   { state_bytes_count += _w.decline_voting_rights_request_object_base_size;     }

   void operator()( const delegate_vesting_shares_operation& op )const
   {
      state_bytes_count += std::max(
         _w.vesting_delegation_object_base_size,
         _w.vesting_delegation_expiration_object_base_size
         );
   }

   void operator()( const escrow_transfer_operation& op )const
   { state_bytes_count += _w.escrow_object_base_size;                            }

   void operator()( const limit_order_create_operation& op )const
   {
      state_bytes_count += op.fill_or_kill ? 0 : _w.limit_order_object_base_size;
      market_op_count++;
   }

   void operator()( const limit_order_create2_operation& op )const
   {
      state_bytes_count += op.fill_or_kill ? 0 : _w.limit_order_object_base_size;
      market_op_count++;
   }

   void operator()( const request_account_recovery_operation& op )const
   {  state_bytes_count += _w.account_recovery_request_object_base_size;         }

   void operator()( const set_withdraw_vesting_route_operation& op )const
   {
      FC_TODO( "Change RC state bytes computation to take SMT's into account" )
      state_bytes_count += _w.withdraw_vesting_route_object_base_size;
   }

   void operator()( const vote_operation& op )const
   {
      FC_TODO( "Change RC state bytes computation to take SMT's into account" )
      state_bytes_count += _w.comment_vote_object_base_size;
   }

   void operator()( const witness_update_operation& op )const
   {
      state_bytes_count +=
           _w.witness_object_base_size
         + _w.witness_object_url_char_size * op.url.size();
   }

   void operator()( const transfer_operation& )const
   {
      FC_TODO( "Change RC state bytes computation to take SMT's into account" )
      market_op_count++;
   }

   void operator()( const transfer_to_vesting_operation& )const
   {
      FC_TODO( "Change RC state bytes computation to take SMT's into account" )
      market_op_count++;
   }

   void operator()( const transfer_to_savings_operation& )const
   {
      FC_TODO( "Change RC state bytes computation to take SMT's into account" )
   }

   void operator()( const transfer_from_savings_operation& )const
   {
      FC_TODO( "Change RC state bytes computation to take SMT's into account" )
   }

   void operator()( const claim_reward_balance_operation& op )const
   {
      FC_TODO( "Change RC state bytes computation to take SMT's into account" )
   }

#ifdef STEEM_ENABLE_SMT
   void operator()( const claim_reward_balance2_operation& op )const
   {
      FC_TODO( "Change RC state bytes computation to take SMT's into account" )
   }
#endif

   void operator()( const withdraw_vesting_operation& op )const
   {
      FC_TODO( "Change RC state bytes computation to take SMT's into account" )
   }

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
   const int64_t tx_size = int64_t( fc::raw::pack_size( tx ) );
   count_operation_visitor vtor(size_info);

   result.resource_count[ resource_history_bytes ] += tx_size;

   for( const operation& op : tx.operations )
      op.visit( vtor );

   result.resource_count[ resource_new_accounts ] += vtor.new_account_op_count;

   if( vtor.market_op_count > 0 )
      result.resource_count[ resource_market_bytes ] += tx_size;

   result.resource_count[ resource_state_bytes ] +=
        size_info.transaction_object_base_size
      + size_info.transaction_object_byte_size * tx_size
      + vtor.state_bytes_count;
}

} } } // steem::plugins::rc

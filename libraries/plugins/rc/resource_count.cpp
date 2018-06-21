
#include <steem/plugins/rc/resource_count.hpp>

#include <steem/protocol/transaction.hpp>

namespace steem { namespace plugins { namespace rc {

using namespace steem::protocol;

struct count_operation_visitor
{
   typedef void result_type;

   mutable int32_t market_op_count = 0;
   mutable int32_t new_account_op_count = 0;

   count_operation_visitor() {}

   template< typename OpType >
   void operator()( const OpType op )const {}

   void operator()( const limit_order_create_operation& )const
   { market_op_count++; }
   void operator()( const limit_order_cancel_operation& )const
   { market_op_count++; }
   void operator()( const transfer_operation& )const
   { market_op_count++; }
   void operator()( const transfer_to_vesting_operation& )const
   { market_op_count++; }

   void operator()( const account_create_operation& op )const
   { new_account_op_count++; }
   void operator()( const account_create_with_delegation_operation& op )const
   { new_account_op_count++; }

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
   const int64_t tx_size = int64_t( fc::raw::pack_size( tx ) );
   count_operation_visitor vtor;

   result.resource_count[ resource_history_bytes ] += tx_size;

   for( const operation& op : tx.operations )
      op.visit( vtor );

   result.resource_count[ resource_new_accounts ] += vtor.new_account_op_count;

   if( vtor.market_op_count > 0 )
      result.resource_count[ resource_market_bytes ] += tx_size;
}

} } } // steem::plugins::rc

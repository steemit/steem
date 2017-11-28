#include <steem/plugins/account_history_api/account_history_api_plugin.hpp>
#include <steem/plugins/account_history_api/account_history_api.hpp>

#include <steem/chain/account_object.hpp>
#include <steem/chain/history_object.hpp>

namespace steem {

using chain::account_history_object;
using chain::operation_object;

namespace plugins { namespace account_history {

namespace detail {

class account_history_api_impl
{
   public:
      account_history_api_impl() : _db( appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db() ) {}

      DECLARE_API(
         (get_ops_in_block)
         (get_transaction)
         (get_account_history)
      )

      chain::database& _db;
};

DEFINE_API( account_history_api_impl, get_ops_in_block )
{
   const auto& idx = _db.get_index< chain::operation_index, chain::by_location >();
   auto itr = idx.lower_bound( args.block_num );
   get_ops_in_block_return result;
   while( itr != idx.end() && itr->block == args.block_num )
   {
      api_operation_object temp = *itr;
      if( !args.only_virtual || is_virtual_operation( temp.op ) )
         result.ops.push_back( temp );
      ++itr;
   }
   return result;
}

DEFINE_API( account_history_api_impl, get_transaction )
{
#ifdef SKIP_BY_TX_ID
   FC_ASSERT( false, "This node's operator has disabled operation indexing by transaction_id" );
#else
   const auto& idx = _db.get_index< chain::operation_index, chain::by_transaction_id >();
   auto itr = idx.lower_bound( args.id );
   if( itr != idx.end() && itr->trx_id == args.id )
   {
      auto blk = _db.fetch_block_by_number( itr->block );
      FC_ASSERT( blk.valid() );
      FC_ASSERT( blk->transactions.size() > itr->trx_in_block );
      get_transaction_return result = blk->transactions[itr->trx_in_block];
      result.block_num       = itr->block;
      result.transaction_num = itr->trx_in_block;
      return result;
   }
   FC_ASSERT( false, "Unknown Transaction ${t}", ("t",args.id) );
#endif
}

DEFINE_API( account_history_api_impl, get_account_history )
{
   FC_ASSERT( args.limit <= 10000, "limit of ${l} is greater than maxmimum allowed", ("l",args.limit) );
   FC_ASSERT( args.start >= args.limit, "start must be greater than limit" );

   const chain::account_object* account = _db.find_account(args.account);

   if(account == nullptr)
      return get_account_history_return();

   const auto& idx = _db.get_index<chain::account_history_index, chain::by_account>();
   auto itr = idx.find(account->id);
   if(itr == idx.end())
      return get_account_history_return();

   get_account_history_return result;

   const account_history_object& data = *itr;

   const auto& collectedOps = data.get_ops();
   auto opsCount = collectedOps.size();
   
   auto shift = opsCount > args.start ? opsCount - args.start : 0;
   auto count = opsCount - shift > args.limit ? args.limit : opsCount - shift;

   auto opI = std::next(collectedOps.crbegin(), shift);
   auto endI = std::next(opI, count + 1); /// Last item selected by `count` must be included too

   uint32_t sequence = opsCount - shift;
   for(; opI != endI; ++opI)
   {
      const auto& opId = *opI;
      const operation_object& op = _db.get(opId);
      result.history[sequence] = api_operation_object(op);
      --sequence;
   }
   return result;
}

} // detail

account_history_api::account_history_api(): my( new detail::account_history_api_impl() )
{
   JSON_RPC_REGISTER_API( STEEM_ACCOUNT_HISTORY_API_PLUGIN_NAME );
}

account_history_api::~account_history_api() {}

DEFINE_API( account_history_api, get_ops_in_block )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_ops_in_block( args );
   });
}

DEFINE_API( account_history_api, get_transaction )
{
#ifdef SKIP_BY_TX_ID
   FC_ASSERT( false, "This node's operator has disabled operation indexing by transaction_id" );
#else
   return my->_db.with_read_lock( [&]()
   {
      return my->get_transaction( args );
   });
#endif
}

DEFINE_API( account_history_api, get_account_history )
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_account_history( args );
   });
}

} } } // steem::plugins::account_history

#include <steem/plugins/account_history_api/account_history_api_plugin.hpp>
#include <steem/plugins/account_history_api/account_history_api.hpp>

namespace steem { namespace plugins { namespace account_history {

namespace detail {

class account_history_api_impl
{
   public:
      account_history_api_impl() : _db( appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db() ) {}

      DECLARE_API_IMPL(
         (get_ops_in_block)
         (get_transaction)
         (get_account_history)
      )

      chain::database& _db;
};

DEFINE_API_IMPL( account_history_api_impl, get_ops_in_block )
{
   const auto& idx = _db.get_index< chain::operation_index, chain::by_location >();
   auto itr = idx.lower_bound( args.block_num );

   std::multiset< api_operation_object > tmp_result;

   while( itr != idx.end() && itr->block == args.block_num )
   {
      api_operation_object temp = *itr;
      if( !args.only_virtual || is_virtual_operation( temp.op ) )
         tmp_result.emplace( std::move( temp ) );
      ++itr;
   }

   if( !tmp_result.empty() )
   {
      get_ops_in_block_return result;

      result.ops.resize( tmp_result.size() );
      std::copy( tmp_result.begin(), tmp_result.end(), result.ops.begin() );

      return result;
   }

   return get_ops_in_block_return();
}

DEFINE_API_IMPL( account_history_api_impl, get_transaction )
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

DEFINE_API_IMPL( account_history_api_impl, get_account_history )
{
   FC_ASSERT( args.limit <= 10000, "limit of ${l} is greater than maxmimum allowed", ("l",args.limit) );
   FC_ASSERT( args.start >= args.limit, "start must be greater than limit" );

   const auto& idx = _db.get_index< chain::account_history_index, chain::by_account >();
   auto itr = idx.lower_bound( boost::make_tuple( args.account, args.start ) );
   auto end = idx.upper_bound( boost::make_tuple( args.account, std::max( int64_t(0), int64_t(itr->sequence) - args.limit ) ) );

   get_account_history_return result;
   while( itr != end )
   {
      result.history[ itr->sequence ] = _db.get( itr->op );
      ++itr;
   }

   return result;
}

} // detail

account_history_api::account_history_api(): my( new detail::account_history_api_impl() )
{
   JSON_RPC_REGISTER_API( STEEM_ACCOUNT_HISTORY_API_PLUGIN_NAME );
}

account_history_api::~account_history_api() {}

DEFINE_READ_APIS( account_history_api,
   (get_ops_in_block)
   (get_transaction)
   (get_account_history)
)

} } } // steem::plugins::account_history


#include <steemit/app/application.hpp>
#include <steemit/app/plugin.hpp>
#include <steemit/chain/database.hpp>
#include <steemit/chain/get_config.hpp>
#include <steemit/chain/history_object.hpp>
#include <steemit/plugin/core_info/core_info_api.hpp>

namespace steemit { namespace plugin { namespace core_info {

using api = core_info_api;

namespace detail {

using namespace steemit::chain;

class core_info_api_impl
{
   public:
      core_info_api_impl( const steemit::chain::database& db );

      void get_block_header( const api::get_block_header_args& args, api::get_block_header_ret& result )const;
      void get_block( const api::get_block_args& args, api::get_block_ret& result )const;
      void get_transaction( const api::get_transaction_args& args, api::get_transaction_ret& result )const;
      void get_config( const api::get_config_args& args, api::get_config_ret& result )const;
      void get_dynamic_global_properties( const api::get_dynamic_global_properties_args& args, api::get_dynamic_global_properties_ret& result )const;
      void get_chain_properties( const api::get_chain_properties_args& args, api::get_chain_properties_ret& result )const;

   private:
      const steemit::chain::database& _db;
};

core_info_api_impl::core_info_api_impl( const steemit::chain::database& db ) : _db(db) {}

void core_info_api_impl::get_block_header( const api::get_block_header_args& args, api::get_block_header_ret& result )const
{
   auto b = _db.fetch_block_by_number( args.block_num );
   if( b )
      result.block = *b;
}

void core_info_api_impl::get_block( const api::get_block_args& args, api::get_block_ret& result )const
{
   result.block = _db.fetch_block_by_number( args.block_num );
}

void core_info_api_impl::get_transaction( const api::get_transaction_args& args, api::get_transaction_ret& result )const
{
   const auto& idx = _db.get_index_type<operation_index>().indices().get<by_transaction_id>();
   auto itr = idx.lower_bound( args.txid );
   if( itr != idx.end() && itr->trx_id == args.txid )
   {
      auto blk = _db.fetch_block_by_number( itr->block );
      FC_ASSERT( blk.valid() );
      FC_ASSERT( blk->transactions.size() > itr->trx_in_block );
      result.tx = blk->transactions[itr->trx_in_block];
      result.tx->block_num       = itr->block;
      result.tx->transaction_num = itr->trx_in_block;
      return;
   }
   result.tx.reset();
}

void core_info_api_impl::get_config( const api::get_config_args& args, api::get_config_ret& result )const
{
   result.config = steemit::chain::get_config();
}

void core_info_api_impl::get_dynamic_global_properties( const api::get_dynamic_global_properties_args& args, api::get_dynamic_global_properties_ret& result )const
{
   result.dgp = _db.get( dynamic_global_property_id_type() );
}

void core_info_api_impl::get_chain_properties( const api::get_chain_properties_args& args, api::get_chain_properties_ret& result )const
{
   result.params = _db.get_witness_schedule_object().median_props;
}

} // ns detail

core_info_api::core_info_api( const chain::database& db ) : _my( std::make_shared< detail::core_info_api_impl >( db ) ) {}
core_info_api::core_info_api( const app::application& app ) : core_info_api( *app.chain_database() ) {}

api::get_block_header_ret core_info_api::get_block_header( api::get_block_header_args args )const
{
   api::get_block_header_ret result;
   _my->get_block_header( args, result );
   return result;
}

api::get_block_ret core_info_api::get_block( api::get_block_args args )const
{
   api::get_block_ret result;
   _my->get_block( args, result );
   return result;
}

api::get_transaction_ret core_info_api::get_transaction( api::get_transaction_args args )const
{
   api::get_transaction_ret result;
   _my->get_transaction( args, result );
   return result;
}

api::get_config_ret core_info_api::get_config( api::get_config_args args )const
{
   api::get_config_ret result;
   _my->get_config( args, result );
   return result;
}

api::get_dynamic_global_properties_ret core_info_api::get_dynamic_global_properties( api::get_dynamic_global_properties_args args )const
{
   api::get_dynamic_global_properties_ret result;
   _my->get_dynamic_global_properties( args, result );
   return result;
}

api::get_chain_properties_ret core_info_api::get_chain_properties( api::get_chain_properties_args args )const
{
   api::get_chain_properties_ret result;
   _my->get_chain_properties( args, result );
   return result;
}

} } } // ns steemit::plugin::core_info

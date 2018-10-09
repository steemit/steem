#include <appbase/application.hpp>

#include <steem/plugins/transaction_status_api/transaction_status_api_plugin.hpp>
#include <steem/plugins/transaction_status_api/transaction_status_api.hpp>

#include <steem/plugins/transaction_status/transaction_status_objects.hpp>

namespace steem { namespace plugins { namespace transaction_status_api {

namespace detail {

class transaction_status_api_impl
{
public:
   transaction_status_api_impl() : _db( appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db() ) {}

   DECLARE_API_IMPL( (find_transaction) )

   chain::database& _db;
private:
   uint32_t block_depth = 64'000;
};

DEFINE_API_IMPL( transaction_status_api_impl, find_transaction )
{
   auto last_irreversible_block_num = _db.get_dynamic_global_properties().last_irreversible_block_num;
   auto tso = _db.find< transaction_status::transaction_status_object, transaction_status::by_trx_id >( args.transaction_id );
   if ( tso != nullptr)
   {
      // If we're not within a block
      if ( !tso->block_num )
         return {
            .status = transaction_status::within_mempool
         };

      // If we're in an irreversible block
      if ( tso->block_num <= last_irreversible_block_num )
         return {
            .status = transaction_status::within_irreversible_block,
            .block_num = tso->block_num
         };
      else
         return {
            .status = transaction_status::within_reversible_block,
            .block_num = tso->block_num
         };
   }

   if ( args.expiration.valid() )
   {
      auto expiration = *( args.expiration );

      int64_t earliest_tracked_block_num = _db.get_dynamic_global_properties().head_block_number - block_depth;
      if ( earliest_tracked_block_num > 0 )
      {
         auto earliest_tracked_block = _db.fetch_block_by_number( static_cast<uint32_t>( earliest_tracked_block_num ) );
         if ( expiration < earliest_tracked_block->timestamp )
            return {
               .status = transaction_status::too_old
            };
      }

      auto last_irreversible_block = _db.fetch_block_by_number( last_irreversible_block_num );
      if ( expiration < last_irreversible_block->timestamp )
         return {
            .status = transaction_status::expired_irreversible
         };
      else if ( expiration < fc::time_point::now() )
         return {
            .status = transaction_status::expired_reversible
         };
   }

   return { .status = transaction_status::unknown };
}

} // steem::plugins::transaction_status_api::detail

transaction_status_api::transaction_status_api() : my( std::make_unique< detail::transaction_status_api_impl >() )
{
   JSON_RPC_REGISTER_API( STEEM_TRANSACTION_STATUS_API_PLUGIN_NAME );
}

transaction_status_api::~transaction_status_api() {}

DEFINE_READ_APIS( transaction_status_api, (find_transaction) )

} } } // steem::plugins::transaction_status_api

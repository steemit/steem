#include <steem/plugins/transaction_status_api/transaction_status_api_plugin.hpp>
#include <steem/plugins/transaction_status_api/transaction_status_api.hpp>

#include <steem/plugins/transaction_status/transaction_status_objects.hpp>

namespace steem { namespace plugins { namespace transaction_status_api {

namespace detail {

class transaction_status_api_impl
{
public:
   transaction_status_api_impl() :
      _db( appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db() ),
      _tsp( appbase::app().get_plugin< steem::plugins::transaction_status::transaction_status_plugin >() ) {}

   DECLARE_API_IMPL( (find_transaction) )

   chain::database& _db;
   transaction_status::transaction_status_plugin& _tsp;
};

DEFINE_API_IMPL( transaction_status_api_impl, find_transaction )
{
   uint32_t earliest_tracked_block_num = _tsp.earliest_tracked_block_num();

   // Have we begun tracking?
   if ( _db.head_block_num() >= earliest_tracked_block_num )
   {
      auto last_irreversible_block_num = _db.get_dynamic_global_properties().last_irreversible_block_num;
      auto tso = _db.find< transaction_status::transaction_status_object, transaction_status::by_trx_id >( args.transaction_id );

      // If we are actively tracking this transaction
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
         // We're in a reversible block
         else
            return {
               .status = transaction_status::within_reversible_block,
               .block_num = tso->block_num
            };
      }

      // If the user has provided us with an expiration
      if ( args.expiration.valid() )
      {
         const auto& expiration = *args.expiration;

         // Check if the expiration is before our earliest tracked block plus maximum transaction expiration
         auto earliest_tracked_block = _db.fetch_block_by_number( earliest_tracked_block_num );
         if ( expiration < earliest_tracked_block->timestamp + STEEM_MAX_TIME_UNTIL_EXPIRATION )
            return {
               .status = transaction_status::too_old
            };

         // If the expiration is on or before our last irreversible block
         if ( last_irreversible_block_num > 0 )
         {
            auto last_irreversible_block = _db.fetch_block_by_number( last_irreversible_block_num );
            if ( expiration <= last_irreversible_block->timestamp )
               return {
                  .status = transaction_status::expired_irreversible
               };
         }
         // If our expiration is in the past
         if ( expiration < _db.head_block_time() )
            return {
               .status = transaction_status::expired_reversible
            };
      }
   }

   // Either the user did not provide an expiration or it is not expired and we didn't hear about this transaction
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

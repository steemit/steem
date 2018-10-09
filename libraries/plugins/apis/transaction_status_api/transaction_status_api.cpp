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
};

DEFINE_API_IMPL( transaction_status_api_impl, find_transaction )
{
   bool is_expired = args.expiration < fc::time_point::now();

   auto tso = _db.find< transaction_status::transaction_status_object, transaction_status::by_trx_id >( args.transaction_id );
   if ( tso != nullptr)
   {
      is_expired = tso->expiration < fc::time_point::now();

      // If we're not within a block
      if ( !tso->block_num )
      {
         return {
            .status = transaction_status::within_mempool
         };
      }

      // If we're in an irreversible block
      if ( tso->block_num <= _db.get_dynamic_global_properties().last_irreversible_block_num )
      {
         return {
            .status = is_expired ? transaction_status::expired_within_irreversible_block : transaction_status::within_irreversible_block,
            .block_num = tso->block_num
         };
      }
      else
      {
         return {
            .status = is_expired ? transaction_status::expired_within_reversible_block : transaction_status::within_reversible_block,
            .block_num = tso->block_num
         };
      }
   }

   // We did not find this transaction in our index
   return { .status = is_expired ? transaction_status::expired : transaction_status::unknown };
}

} // steem::plugins::transaction_status_api::detail

transaction_status_api::transaction_status_api() : my( std::make_unique< detail::transaction_status_api_impl >() )
{
   JSON_RPC_REGISTER_API( STEEM_TRANSACTION_STATUS_API_PLUGIN_NAME );
}

transaction_status_api::~transaction_status_api() {}

DEFINE_READ_APIS( transaction_status_api, (find_transaction) )

} } } // steem::plugins::transaction_status_api

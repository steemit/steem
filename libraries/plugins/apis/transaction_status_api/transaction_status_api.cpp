#include <appbase/application.hpp>

#include <steem/plugins/transaction_status_api/transaction_status_api_plugin.hpp>
#include <steem/plugins/transaction_status_api/transaction_status_api.hpp>

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
   find_transaction_return r;
   r.status = transaction_status::unknown;
   return r;
}

} // steem::plugins::transaction_status_api::detail

transaction_status_api::transaction_status_api() : my( std::make_unique< detail::transaction_status_api_impl >() )
{
   JSON_RPC_REGISTER_API( STEEM_TRANSACTION_STATUS_API_PLUGIN_NAME );
}

transaction_status_api::~transaction_status_api() {}

DEFINE_READ_APIS( transaction_status_api, (find_transaction) )

} } } // steem::plugins::transaction_status_api

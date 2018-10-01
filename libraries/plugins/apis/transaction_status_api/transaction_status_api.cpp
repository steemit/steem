#include <appbase/application.hpp>

#include <steem/plugins/transaction_status_api/transaction_status_api_plugin.hpp>
#include <steem/plugins/transaction_status_api/transaction_status_api.hpp>

#include <steem/protocol/get_config.hpp>
#include <steem/protocol/exceptions.hpp>
#include <steem/protocol/transaction_util.hpp>

#include <steem/utilities/git_revision.hpp>

#include <fc/git_revision.hpp>

namespace steem { namespace plugins { namespace transaction_status_api {

namespace detail {

class transaction_status_api_impl
{
   public:
      transaction_status_api_impl();
      ~transaction_status_api_impl();

      DECLARE_API_IMPL
      (
         (find_transaction)
      )

      chain::database& _db;
};

DEFINE_API_IMPL( transaction_status_api_impl, find_transaction )
{
   find_transaction_return r;
   r.status = transaction_status::unknown;
   return r;
}

} // steem::plugins::transaction_status_api::detail

transaction_status_api::transaction_status_api()
   : my( std::make_unique< transaction_status_api_impl >() )
{
   JSON_RPC_REGISTER_API( STEEM_TRANSACTION_STATUS_API_PLUGIN_NAME );
}

transaction_status_api::~transaction_status_api() {}

DEFINE_READ_APIS( transaction_status_api,
   (find_transaction)
)

} } } // steem::plugins::transaction_status_api

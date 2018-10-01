#include <steem/plugins/transaction_status_api/transaction_status_api.hpp>

namespace steem { namespace plugins { namespace transaction_status_api {

transaction_status_api::transaction_status_api() {}
transaction_status_api::~transaction_status_api() {}

void transaction_status_api::set_program_options(
   options_description& cli,
   options_description& cfg ) {}

void transaction_status_api::plugin_initialize( const variables_map& options )
{
   api = std::make_shared< transaction_status_api >();
}

void transaction_status_api::plugin_startup() {}

void transaction_status_api::plugin_shutdown() {}

} } } // steem::plugins::transaction_status_api

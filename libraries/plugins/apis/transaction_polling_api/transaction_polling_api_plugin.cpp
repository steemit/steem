#include <steem/plugins/transaction_polling_api/transaction_polling_api.hpp>
#include <steem/plugins/transaction_polling_api/transaction_polling_api_plugin.hpp>

namespace steem { namespace plugins { namespace transaction_polling_api {

transaction_polling_api_plugin::transaction_polling_api_plugin() {}
transaction_polling_api_plugin::~transaction_polling_api_plugin() {}

void transaction_polling_api_plugin::set_program_options( options_description& cli, options_description& cfg ) {}

void transaction_polling_api_plugin::plugin_initialize( const variables_map& options )
{
   api = std::make_shared< transaction_polling_api >();
}

void transaction_polling_api_plugin::plugin_startup() {}

void transaction_polling_api_plugin::plugin_shutdown() {}

} } } // steem::plugins::transaction_polling_api

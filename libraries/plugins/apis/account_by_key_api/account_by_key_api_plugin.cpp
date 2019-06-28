#include <steem/plugins/account_by_key_api/account_by_key_api_plugin.hpp>
#include <steem/plugins/account_by_key_api/account_by_key_api.hpp>


namespace steem { namespace plugins { namespace account_by_key {

account_by_key_api_plugin::account_by_key_api_plugin() {}
account_by_key_api_plugin::~account_by_key_api_plugin() {}

void account_by_key_api_plugin::set_program_options( options_description& cli, options_description& cfg ) {}

void account_by_key_api_plugin::plugin_initialize( const variables_map& options )
{
   api = std::make_shared< account_by_key_api >();
}

void account_by_key_api_plugin::plugin_startup() {}
void account_by_key_api_plugin::plugin_shutdown() {}

} } } // steem::plugins::account_by_key

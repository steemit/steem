#include <steemit/plugins/account_by_key_api/account_by_key_api_plugin.hpp>
#include <steemit/plugins/account_by_key_api/account_by_key_api.hpp>


namespace steemit { namespace plugins { namespace account_by_key {

account_by_key_api_plugin::account_by_key_api_plugin() : appbase::plugin< account_by_key_api_plugin >( ACCOUNT_BY_KEY_API_PLUGIN_NAME ) {}
account_by_key_api_plugin::~account_by_key_api_plugin() {}

void account_by_key_api_plugin::set_program_options( options_description& cli, options_description& cfg ) {}

void account_by_key_api_plugin::plugin_initialize( const variables_map& options )
{
   api = std::make_shared< account_by_key_api >();
}

void account_by_key_api_plugin::plugin_startup() {}
void account_by_key_api_plugin::plugin_shutdown() {}

} } } // steemit::plugins::account_by_key

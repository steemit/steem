#include <dpn/plugins/account_history_api/account_history_api_plugin.hpp>
#include <dpn/plugins/account_history_api/account_history_api.hpp>


namespace dpn { namespace plugins { namespace account_history {

account_history_api_plugin::account_history_api_plugin() {}
account_history_api_plugin::~account_history_api_plugin() {}

void account_history_api_plugin::set_program_options( options_description& cli, options_description& cfg ) {}

void account_history_api_plugin::plugin_initialize( const variables_map& options )
{
   api = std::make_shared< account_history_api >();
}

void account_history_api_plugin::plugin_startup() {}
void account_history_api_plugin::plugin_shutdown() {}

} } } // dpn::plugins::account_history

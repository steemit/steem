#include <steemit/plugins/condenser_api/condenser_api_plugin.hpp>
#include <steemit/plugins/condenser_api/condenser_api.hpp>
#include <steemit/plugins/chain/chain_plugin.hpp>

namespace steemit { namespace plugins { namespace condenser_api {

condenser_api_plugin::condenser_api_plugin() : appbase::plugin< condenser_api_plugin >( CONDENSER_API_PLUGIN_NAME ) {}
condenser_api_plugin::~condenser_api_plugin() {}

void condenser_api_plugin::set_program_options( options_description& cli, options_description& cfg )
{
   cli.add_options()
      ("disable-get-block", "Disable get_block API call" )
      ;
}

void condenser_api_plugin::plugin_initialize( const variables_map& options )
{
   bool disable_get_block = options.count( "disable-get-block" );
   auto& chain_plugin = appbase::app().get_plugin< steemit::plugins::chain::chain_plugin >();

   db_api = std::make_unique< condenser_api >( chain_plugin.db(), disable_get_block );
}

void condenser_api_plugin::plugin_startup() {}
void condenser_api_plugin::plugin_shutdown() {}

} } } // steemit::plugins::condenser_api

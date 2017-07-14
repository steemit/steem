#include <steemit/plugins/database_api/database_api_plugin.hpp>
#include <steemit/plugins/database_api/database_api.hpp>
#include <steemit/plugins/chain/chain_plugin.hpp>

#include <steemit/app/database_api.hpp>

namespace steemit { namespace plugins { namespace database_api {

database_api_plugin::database_api_plugin() : appbase::plugin< database_api_plugin >( DATABASE_API_PLUGIN_NAME ) {}
database_api_plugin::~database_api_plugin() {}

void database_api_plugin::set_program_options( options_description& cli, options_description& cfg )
{
   cli.add_options()
      ("disable-get-block", "Disable get_block API call" )
      ;
}

void database_api_plugin::plugin_initialize( const variables_map& options )
{
   bool disable_get_block = options.count( "disable-get-block" );
   auto& chain_plugin = appbase::app().get_plugin< steemit::plugins::chain::chain_plugin >();

   db_api = std::make_unique< database_api >( chain_plugin.db(), disable_get_block );
}

void database_api_plugin::plugin_startup() {}
void database_api_plugin::plugin_shutdown() {}

} } } // steemit::plugins::test_api

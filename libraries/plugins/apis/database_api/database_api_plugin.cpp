#include <steemit/plugins/database_api/database_api.hpp>
#include <steemit/plugins/database_api/database_api_plugin.hpp>

namespace steemit { namespace plugins { namespace database_api {

database_api_plugin::database_api_plugin() :
   appbase::plugin< database_api_plugin >( STEEM_DATABASE_API_PLUGIN_NAME ) {}

database_api_plugin::~database_api_plugin(){}

void database_api_plugin::set_program_options(
   options_description& cli,
   options_description& cfg ) {}

void database_api_plugin::plugin_initialize( const variables_map& options )
{
   api = std::make_shared< database_api >();
}

void database_api_plugin::plugin_startup() {}

void database_api_plugin::plugin_shutdown() {}

} } } // steemit::plugins::database_api

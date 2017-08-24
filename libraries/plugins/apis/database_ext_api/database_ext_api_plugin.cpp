#include <steemit/plugins/database_ext_api/database_ext_api.hpp>
#include <steemit/plugins/database_ext_api/database_ext_api_plugin.hpp>

namespace steemit { namespace plugins { namespace database_ext_api {

database_ext_api_plugin::database_ext_api_plugin() {}
database_ext_api_plugin::~database_ext_api_plugin() {}

void database_ext_api_plugin::set_program_options(
   options_description& cli,
   options_description& cfg ) {}

void database_ext_api_plugin::plugin_initialize( const variables_map& options )
{
   api = std::make_shared< database_ext_api >();
}

void database_ext_api_plugin::plugin_startup() {}

void database_ext_api_plugin::plugin_shutdown() {}

} } } // steemit::plugins::database_ext_api

#include <steemit/db_api/db_api.hpp>
#include <steemit/db_api/db_api_plugin.hpp>

namespace steemit { namespace db_api {

void db_api_plugin::plugin_startup()
{
   app().register_api_factory< db_api >( "db_api" );
}

} } // steemit::db_api

STEEMIT_DEFINE_PLUGIN( db_api, steemit::db_api::db_api_plugin )

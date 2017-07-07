#pragma once
#include <steemit/plugins/api_register/api_register_plugin.hpp>

#include <appbase/application.hpp>

#include <steemit/app/database_api.hpp>

#define DATABASE_API_PLUGIN_NAME "database_api"

namespace steemit { namespace plugins { namespace database_api {

using namespace appbase;

class database_api_plugin : public appbase::plugin< database_api_plugin >
{
public:
   APPBASE_PLUGIN_REQUIRES( (steemit::plugins::api_register::api_register_plugin) )

   database_api_plugin();
   virtual ~database_api_plugin();

   std::string get_name(){ return DATABASE_API_PLUGIN_NAME; }
   virtual void set_program_options( options_description& cli, options_description& cfg ) override;

   void plugin_initialize( const variables_map& options );
   void plugin_startup();
   void plugin_shutdown();

private:
   std::unique_ptr< class database_api > db_api;
};

} } } // steemit::plugins::database_api

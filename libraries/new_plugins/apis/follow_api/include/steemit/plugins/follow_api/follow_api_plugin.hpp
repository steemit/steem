#pragma once
#include <steemit/plugins/follow/follow_plugin.hpp>
#include <steemit/plugins/json_rpc/json_rpc_plugin.hpp>

#include <appbase/application.hpp>

#define FOLLOW_API_PLUGIN_NAME "follow_api"


namespace steemit { namespace plugins { namespace follow_api {

using namespace appbase;

class follow_api_plugin : public appbase::plugin< follow_api_plugin >
{
public:
   APPBASE_PLUGIN_REQUIRES(
      (steemit::plugins::follow::follow_plugin)
      (steemit::plugins::json_rpc::json_rpc_plugin)
   )

   follow_api_plugin();
   virtual ~follow_api_plugin();

   virtual void set_program_options( options_description& cli, options_description& cfg ) override;

   void plugin_initialize( const variables_map& options );
   void plugin_startup();
   void plugin_shutdown();

private:
   std::unique_ptr< class follow_api > _api;
};

} } } // steemit::plugins::follow_api

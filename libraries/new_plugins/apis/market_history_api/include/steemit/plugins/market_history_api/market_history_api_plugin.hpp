#pragma once
#include <steemit/plugins/market_history/market_history_plugin.hpp>
#include <steemit/plugins/json_rpc/json_rpc_plugin.hpp>

#include <appbase/application.hpp>

#define MARKET_HISTORY_API_PLUGIN_NAME "market_history_api"


namespace steemit { namespace plugins { namespace market_history_api {

using namespace appbase;

class market_history_api_plugin : public appbase::plugin< market_history_api_plugin >
{
public:
   APPBASE_PLUGIN_REQUIRES(
      (steemit::plugins::market_history::market_history_plugin)
      (steemit::plugins::json_rpc::json_rpc_plugin)
   )

   market_history_api_plugin();
   virtual ~market_history_api_plugin();

   virtual void set_program_options( options_description& cli, options_description& cfg ) override;

   void plugin_initialize( const variables_map& options );
   void plugin_startup();
   void plugin_shutdown();

private:
   std::unique_ptr< class market_history_api > _api;
};

} } } // steemit::plugins::market_history_api

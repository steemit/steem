#pragma once
#include <steem/plugins/market_history/market_history_plugin.hpp>
#include <steem/plugins/json_rpc/json_rpc_plugin.hpp>

#include <appbase/application.hpp>

#define STEEM_MARKET_HISTORY_API_PLUGIN_NAME "market_history_api"


namespace steem { namespace plugins { namespace market_history {

using namespace appbase;

class market_history_api_plugin : public appbase::plugin< market_history_api_plugin >
{
public:
   APPBASE_PLUGIN_REQUIRES(
      (steem::plugins::market_history::market_history_plugin)
      (steem::plugins::json_rpc::json_rpc_plugin)
   )

   market_history_api_plugin();
   virtual ~market_history_api_plugin();

   static const std::string& name() { static std::string name = STEEM_MARKET_HISTORY_API_PLUGIN_NAME; return name; }

   virtual void set_program_options( options_description& cli, options_description& cfg ) override;

   virtual void plugin_initialize( const variables_map& options ) override;
   virtual void plugin_startup() override;
   virtual void plugin_shutdown() override;

   std::shared_ptr< class market_history_api > api;
};

} } } // steem::plugins::market_history

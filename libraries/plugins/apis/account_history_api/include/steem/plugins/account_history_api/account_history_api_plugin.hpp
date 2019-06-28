#pragma once
#include <steem/chain/steem_fwd.hpp>
#include <steem/plugins/account_history/account_history_plugin.hpp>
#include <steem/plugins/json_rpc/json_rpc_plugin.hpp>

#include <appbase/application.hpp>

#define STEEM_ACCOUNT_HISTORY_API_PLUGIN_NAME "account_history_api"


namespace steem { namespace plugins { namespace account_history {

using namespace appbase;

class account_history_api_plugin : public plugin< account_history_api_plugin >
{
public:
   APPBASE_PLUGIN_REQUIRES(
      (steem::plugins::json_rpc::json_rpc_plugin)
   )

   account_history_api_plugin();
   virtual ~account_history_api_plugin();

   static const std::string& name() { static std::string name = STEEM_ACCOUNT_HISTORY_API_PLUGIN_NAME; return name; }

   virtual void set_program_options( options_description& cli, options_description& cfg ) override;

   virtual void plugin_initialize( const variables_map& options ) override;
   virtual void plugin_startup() override;
   virtual void plugin_shutdown() override;

   std::shared_ptr< class account_history_api > api;
};

} } } // steem::plugins::account_history

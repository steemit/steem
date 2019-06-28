#pragma once
#include <dpn/chain/dpn_fwd.hpp>
#include <dpn/plugins/account_history/account_history_plugin.hpp>
#include <dpn/plugins/json_rpc/json_rpc_plugin.hpp>

#include <appbase/application.hpp>

#define DPN_ACCOUNT_HISTORY_API_PLUGIN_NAME "account_history_api"


namespace dpn { namespace plugins { namespace account_history {

using namespace appbase;

class account_history_api_plugin : public plugin< account_history_api_plugin >
{
public:
   APPBASE_PLUGIN_REQUIRES(
      (dpn::plugins::json_rpc::json_rpc_plugin)
   )

   account_history_api_plugin();
   virtual ~account_history_api_plugin();

   static const std::string& name() { static std::string name = DPN_ACCOUNT_HISTORY_API_PLUGIN_NAME; return name; }

   virtual void set_program_options( options_description& cli, options_description& cfg ) override;

   virtual void plugin_initialize( const variables_map& options ) override;
   virtual void plugin_startup() override;
   virtual void plugin_shutdown() override;

   std::shared_ptr< class account_history_api > api;
};

} } } // dpn::plugins::account_history

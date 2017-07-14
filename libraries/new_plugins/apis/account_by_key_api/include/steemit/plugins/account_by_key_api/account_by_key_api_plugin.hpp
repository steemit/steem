#pragma once
#include <steemit/plugins/account_by_key/account_by_key_plugin.hpp>
#include <steemit/plugins/json_rpc/json_rpc_plugin.hpp>

#include <appbase/application.hpp>

#define ACCOUNT_BY_KEY_API_PLUGIN_NAME "account_by_key_api"


namespace steemit { namespace plugins { namespace account_by_key_api {

using namespace appbase;

class account_by_key_api_plugin : public appbase::plugin< account_by_key_api_plugin >
{
public:
   APPBASE_PLUGIN_REQUIRES(
      (steemit::plugins::account_by_key::account_by_key_plugin)
      (steemit::plugins::json_rpc::json_rpc_plugin)
   )

   account_by_key_api_plugin();
   virtual ~account_by_key_api_plugin();

   std::string get_name(){ return ACCOUNT_BY_KEY_API_PLUGIN_NAME; }
   virtual void set_program_options( options_description& cli, options_description& cfg ) override;

   void plugin_initialize( const variables_map& options );
   void plugin_startup();
   void plugin_shutdown();

private:
   std::unique_ptr< class account_by_key_api > _api;
};

} } } // steemit::plugins::account_by_key_api

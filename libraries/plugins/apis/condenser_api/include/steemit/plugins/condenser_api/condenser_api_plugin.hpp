#pragma once
#include <steemit/plugins/json_rpc/json_rpc_plugin.hpp>
#include <steemit/plugins/chain/chain_plugin.hpp>

#include <appbase/application.hpp>

#define CONDENSER_API_PLUGIN_NAME "condenser_api"

namespace steemit { namespace plugins { namespace condenser_api {

using namespace appbase;

class condenser_api_plugin : public appbase::plugin< condenser_api_plugin >
{
public:
   APPBASE_PLUGIN_REQUIRES( (steemit::plugins::json_rpc::json_rpc_plugin)(steemit::plugins::chain::chain_plugin) )

   condenser_api_plugin();
   virtual ~condenser_api_plugin();

   std::string get_name(){ return condenser_API_PLUGIN_NAME; }
   virtual void set_program_options( options_description& cli, options_description& cfg ) override;

   void plugin_initialize( const variables_map& options );
   void plugin_startup();
   void plugin_shutdown();

private:
   std::unique_ptr< class condenser_api > db_api;
};

} } } // steemit::plugins::condenser_api

#pragma once
#include <steemit/plugins/json_rpc/json_rpc_plugin.hpp>
#include <steemit/plugins/chain/chain_plugin.hpp>
#include <steemit/plugins/p2p/p2p_plugin.hpp>

#include <appbase/application.hpp>

#define NETWORK_BROADCAST_API_PLUGIN_NAME "network_broadcast_api"

namespace steemit { namespace plugins { namespace network_broadcast_api {

using namespace appbase;

class network_broadcast_api_plugin : public appbase::plugin< network_broadcast_api_plugin >
{
public:
   APPBASE_PLUGIN_REQUIRES(
      (steemit::plugins::json_rpc::json_rpc_plugin)
      (steemit::plugins::chain::chain_plugin)
      (steemit::plugins::p2p::p2p_plugin)
   )

   network_broadcast_api_plugin();
   virtual ~network_broadcast_api_plugin();

   std::string get_name(){ return NETWORK_BROADCAST_API_PLUGIN_NAME; }
   virtual void set_program_options( options_description& cli, options_description& cfg ) override;

   void plugin_initialize( const variables_map& options );
   void plugin_startup();
   void plugin_shutdown();

private:
   std::unique_ptr< class network_broadcast_api > nb_api;
};

} } } // steemit::plugins::network_broadcast_api

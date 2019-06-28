#pragma once
#include <dpn/chain/dpn_fwd.hpp>
#include <dpn/plugins/json_rpc/json_rpc_plugin.hpp>
#include <dpn/plugins/chain/chain_plugin.hpp>
#include <dpn/plugins/p2p/p2p_plugin.hpp>
#include <dpn/plugins/rc/rc_plugin.hpp>

#include <appbase/application.hpp>

#define DPN_NETWORK_BROADCAST_API_PLUGIN_NAME "network_broadcast_api"

namespace dpn { namespace plugins { namespace network_broadcast_api {

using namespace appbase;

class network_broadcast_api_plugin : public appbase::plugin< network_broadcast_api_plugin >
{
public:
   APPBASE_PLUGIN_REQUIRES(
      (dpn::plugins::json_rpc::json_rpc_plugin)
      (dpn::plugins::rc::rc_plugin)
      (dpn::plugins::chain::chain_plugin)
      (dpn::plugins::p2p::p2p_plugin)
   )

   network_broadcast_api_plugin();
   virtual ~network_broadcast_api_plugin();

   static const std::string& name() { static std::string name = DPN_NETWORK_BROADCAST_API_PLUGIN_NAME; return name; }

   virtual void set_program_options( options_description& cli, options_description& cfg ) override;
   virtual void plugin_initialize( const variables_map& options ) override;
   virtual void plugin_startup() override;
   virtual void plugin_shutdown() override;

   std::shared_ptr< class network_broadcast_api > api;
};

} } } // dpn::plugins::network_broadcast_api

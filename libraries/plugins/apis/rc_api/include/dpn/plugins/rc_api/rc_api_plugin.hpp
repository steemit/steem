#pragma once
#include <dpn/chain/dpn_fwd.hpp>
#include <dpn/plugins/rc/rc_plugin.hpp>
#include <dpn/plugins/json_rpc/json_rpc_plugin.hpp>

#include <appbase/application.hpp>

#define DPN_RC_API_PLUGIN_NAME "rc_api"


namespace dpn { namespace plugins { namespace rc {

using namespace appbase;

class rc_api_plugin : public appbase::plugin< rc_api_plugin >
{
public:
   APPBASE_PLUGIN_REQUIRES(
      (dpn::plugins::rc::rc_plugin)
      (dpn::plugins::json_rpc::json_rpc_plugin)
   )

   rc_api_plugin();
   virtual ~rc_api_plugin();

   static const std::string& name() { static std::string name = DPN_RC_API_PLUGIN_NAME; return name; }

   virtual void set_program_options( options_description& cli, options_description& cfg ) override;

   virtual void plugin_initialize( const variables_map& options ) override;
   virtual void plugin_startup() override;
   virtual void plugin_shutdown() override;

   std::shared_ptr< class rc_api > api;
};

} } } // dpn::plugins::rc

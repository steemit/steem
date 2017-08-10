#pragma once
#include <steemit/plugins/witness/witness_plugin.hpp>
#include <steemit/plugins/json_rpc/json_rpc_plugin.hpp>

#include <appbase/application.hpp>

#define STEEM_WITNESS_API_PLUGIN_NAME "witness_api"


namespace steemit { namespace plugins { namespace witness {

using namespace appbase;

class witness_api_plugin : public appbase::plugin< witness_api_plugin >
{
public:
   APPBASE_PLUGIN_REQUIRES(
      (steemit::plugins::witness::witness_plugin)
      (steemit::plugins::json_rpc::json_rpc_plugin)
   )

   witness_api_plugin();
   virtual ~witness_api_plugin();

   static const std::string& name() { static std::string name = STEEM_WITNESS_API_PLUGIN_NAME; return name; }

   virtual void set_program_options( options_description& cli, options_description& cfg ) override;

   void plugin_initialize( const variables_map& options );
   void plugin_startup();
   void plugin_shutdown();

   std::shared_ptr< class witness_api > api;
};

} } } // steemit::plugins::witness

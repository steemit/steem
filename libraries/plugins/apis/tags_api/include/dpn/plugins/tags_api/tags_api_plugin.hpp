#pragma once
#include <dpn/chain/dpn_fwd.hpp>

#include <dpn/plugins/tags/tags_plugin.hpp>
#include <dpn/plugins/json_rpc/json_rpc_plugin.hpp>

#include <appbase/application.hpp>

#define DPN_TAGS_API_PLUGIN_NAME "tags_api"


namespace dpn { namespace plugins { namespace tags {

using namespace appbase;

class tags_api_plugin : public appbase::plugin< tags_api_plugin >
{
public:
   APPBASE_PLUGIN_REQUIRES(
      (dpn::plugins::tags::tags_plugin)
      (dpn::plugins::json_rpc::json_rpc_plugin)
   )

   tags_api_plugin();
   virtual ~tags_api_plugin();

   static const std::string& name() { static std::string name = DPN_TAGS_API_PLUGIN_NAME; return name; }

   virtual void set_program_options( options_description& cli, options_description& cfg ) override;

   virtual void plugin_initialize( const variables_map& options ) override;
   virtual void plugin_startup() override;
   virtual void plugin_shutdown() override;

   std::shared_ptr< class tags_api > api;
};

} } } // dpn::plugins::tags

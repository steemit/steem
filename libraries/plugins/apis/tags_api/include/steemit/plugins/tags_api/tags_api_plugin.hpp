#pragma once
#include <steemit/plugins/tags/tags_plugin.hpp>
#include <steemit/plugins/json_rpc/json_rpc_plugin.hpp>

#include <appbase/application.hpp>

#define STEEM_TAGS_API_PLUGIN_NAME "tags_api"


namespace steemit { namespace plugins { namespace tags {

using namespace appbase;

class tags_api_plugin : public appbase::plugin< tags_api_plugin >
{
public:
   APPBASE_PLUGIN_REQUIRES(
      (steemit::plugins::tags::tags_plugin)
      (steemit::plugins::json_rpc::json_rpc_plugin)
   )

   tags_api_plugin();
   virtual ~tags_api_plugin();

   std::string get_name(){ return STEEM_TAGS_API_PLUGIN_NAME; }
   virtual void set_program_options( options_description& cli, options_description& cfg ) override;

   void plugin_initialize( const variables_map& options );
   void plugin_startup();
   void plugin_shutdown();

   std::shared_ptr< class tags_api > api;
};

} } } // steemit::plugins::tags

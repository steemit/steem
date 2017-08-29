#pragma once
#include <steemit/plugins/chain/chain_plugin.hpp>
#include <steemit/plugins/json_rpc/json_rpc_plugin.hpp>

#include <appbase/application.hpp>

namespace steemit { namespace plugins { namespace block_api {

using namespace appbase;

#define STEEM_BLOCK_API_PLUGIN_NAME "block_api"

class block_api_plugin : public plugin< block_api_plugin >
{
   public:
      block_api_plugin();
      virtual ~block_api_plugin();

      APPBASE_PLUGIN_REQUIRES(
         (steemit::plugins::json_rpc::json_rpc_plugin)
         (steemit::plugins::chain::chain_plugin)
      )

      static const std::string& name() { static std::string name = STEEM_BLOCK_API_PLUGIN_NAME; return name; }

      virtual void set_program_options(
         options_description& cli,
         options_description& cfg ) override;
      void plugin_initialize( const variables_map& options );
      void plugin_startup();
      void plugin_shutdown();

      std::shared_ptr< class block_api > api;
};

} } } // steemit::plugins::block_api

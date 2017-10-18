#pragma once
#include <steem/plugins/chain/chain_plugin.hpp>
#include <steem/plugins/json_rpc/json_rpc_plugin.hpp>

#include <appbase/application.hpp>

namespace steem { namespace plugins { namespace token_api {

using namespace appbase;

#define STEEM_TOKEN_API_PLUGIN_NAME "token_api"

class token_api_plugin : public plugin< token_api_plugin >
{
   public:
      token_api_plugin();
      virtual ~token_api_plugin();

      APPBASE_PLUGIN_REQUIRES(
         (steem::plugins::json_rpc::json_rpc_plugin)
         (steem::plugins::chain::chain_plugin)
      )

      static const std::string& name() { static std::string name = STEEM_TOKEN_API_PLUGIN_NAME; return name; }

      virtual void set_program_options(
         options_description& cli,
         options_description& cfg ) override;
      void plugin_initialize( const variables_map& options ) override;
      void plugin_startup() override;
      void plugin_shutdown() override;

      std::shared_ptr< class token_api > api;
};

} } } // steem::plugins::token_api

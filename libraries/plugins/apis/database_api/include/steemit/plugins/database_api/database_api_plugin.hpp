#pragma once
#include <steemit/plugins/chain/chain_plugin.hpp>
#include <steemit/plugins/json_rpc/json_rpc_plugin.hpp>

#include <appbase/application.hpp>

namespace steemit { namespace plugins { namespace database_api {

using namespace appbase;

#define STEEM_DATABASE_API_PLUGIN_NAME "database_api"

class database_api_plugin : public plugin< database_api_plugin >
{
   public:
      database_api_plugin();
      virtual ~database_api_plugin();

      APPBASE_PLUGIN_REQUIRES(
         (steemit::plugins::json_rpc::json_rpc_plugin)
         (steemit::plugins::chain::chain_plugin)
      )

      static const std::string& name() { static std::string name = STEEM_DATABASE_API_PLUGIN_NAME; return name; }

      virtual void set_program_options(
         options_description& cli,
         options_description& cfg ) override;
      virtual void plugin_initialize( const variables_map& options ) override;
      virtual void plugin_startup() override;
      virtual void plugin_shutdown() override;

      std::shared_ptr< class database_api > api;
};

} } } // steemit::plugins::database_api

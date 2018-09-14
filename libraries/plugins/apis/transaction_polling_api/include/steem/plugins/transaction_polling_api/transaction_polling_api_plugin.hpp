#pragma once
#include <steem/plugins/chain/chain_plugin.hpp>
#include <steem/plugins/json_rpc/json_rpc_plugin.hpp>

#include <appbase/application.hpp>

namespace steem { namespace plugins { namespace transaction_polling_api {

using namespace appbase;

#define STEEM_TRANSACTION_POLLING_API_PLUGIN_NAME "transaction_polling_api"

class transaction_polling_api_plugin : public appbase::plugin< transaction_polling_api_plugin >
{
public:
   APPBASE_PLUGIN_REQUIRES(
      (steem::plugins::json_rpc::json_rpc_plugin)
      (steem::plugins::chain::chain_plugin)
   )

   transaction_polling_api_plugin();
   virtual ~transaction_polling_api_plugin();

   static const std::string& name() { static std::string name = STEEM_TRANSACTION_POLLING_API_PLUGIN_NAME; return name; }

   virtual void set_program_options( options_description& cli, options_description& cfg ) override;

   virtual void plugin_initialize( const variables_map& options ) override;
   virtual void plugin_startup() override;
   virtual void plugin_shutdown() override;

   std::shared_ptr< class transaction_polling_api > api;
};

} } } // steem::plugins::transaction_polling

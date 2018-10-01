#pragma once
#include <steem/plugins/chain/chain_plugin.hpp>

#include <appbase/application.hpp>

namespace steem { namespace plugins { namespace transaction_status {

#define STEEM_TRANSACTION_STATUS_PLUGIN_NAME "transaction_status"

class transaction_status_impl;

class transaction_status_plugin : public appbase::plugin< transaction_status_plugin >
{
   public:
      transaction_status_plugin();
      virtual ~transaction_status_plugin();

      APPBASE_PLUGIN_REQUIRES(
         (steem::plugins::chain::chain_plugin)
      )

      static const std::string& name() { static std::string name = STEEM_TRANSACTION_STATUS_PLUGIN_NAME; return name; }

      virtual void set_program_options(
         options_description& cli,
         options_description& cfg ) override;
      virtual void plugin_initialize( const variables_map& options ) override;
      virtual void plugin_startup() override;
      virtual void plugin_shutdown() override;
   private:
      std::shared_ptr< class transaction_status_impl > my;
};

} } } // steem::plugins::transaction_status

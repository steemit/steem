#pragma once
#include <appbase/application.hpp>

#include <steemit/plugins/chain/chain_plugin.hpp>

namespace steemit { namespace plugins { namespace account_by_key {

using namespace appbase;

#define ACCOUNT_BY_KEY_PLUGIN_NAME "account_by_key"

class account_by_key_plugin : public appbase::plugin< account_by_key_plugin >
{
   public:
      account_by_key_plugin();
      virtual ~account_by_key_plugin();

      APPBASE_PLUGIN_REQUIRES( (steemit::plugins::chain::chain_plugin) )

      virtual void set_program_options( options_description& cli, options_description& cfg ) override;
      void plugin_initialize( const variables_map& options );
      void plugin_startup();
      void plugin_shutdown();

      std::unique_ptr< class account_by_key_plugin_impl > my;
};

} } } // steemit::plugins::account_by_key

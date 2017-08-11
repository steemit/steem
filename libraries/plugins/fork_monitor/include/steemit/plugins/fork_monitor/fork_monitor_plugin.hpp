#pragma once

#include <steemit/plugins/chain/chain_plugin.hpp>
#include <appbase/application.hpp>

namespace steemit { namespace plugins { namespace fork_monitor {

using namespace appbase;

#define STEEM_fork_monitor_plugin_NAME "fork_monitor"

namespace detail{ class fork_monitor_plugin_impl; }

class fork_monitor_plugin : public appbase::plugin< fork_monitor_plugin >
{
   public:
      fork_monitor_plugin();
      virtual ~fork_monitor_plugin();

      APPBASE_PLUGIN_REQUIRES( (steemit::plugins::chain::chain_plugin) )

      static const std::string& name() { static std::string name = STEEM_fork_monitor_plugin_NAME; return name; }

      virtual void set_program_options( options_description& cli, options_description& cfg ) override;
      void plugin_initialize( const variables_map& options );
      void plugin_startup();
      void plugin_shutdown();

      std::unique_ptr< detail::fork_monitor_plugin_impl > _my;
};

} } } // steemit::plugins::account_by_key

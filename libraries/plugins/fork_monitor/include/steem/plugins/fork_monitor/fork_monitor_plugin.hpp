#pragma once

#include <steem/plugins/chain/chain_plugin.hpp>
#include <appbase/application.hpp>

namespace steem { namespace plugins { namespace fork_monitor {

using namespace appbase;
using namespace steem::chain;

#define STEEM_FORK_MONITOR_PLUGIN_NAME "fork_monitor"

namespace detail{ class fork_monitor_plugin_impl; }

class fork_monitor_plugin : public plugin< fork_monitor_plugin >
{
   public:
      fork_monitor_plugin();
      virtual ~fork_monitor_plugin();

      APPBASE_PLUGIN_REQUIRES( (steem::plugins::chain::chain_plugin) );

      static const std::string& name() { static std::string name = STEEM_FORK_MONITOR_PLUGIN_NAME; return name; }

      virtual void set_program_options( options_description& cli, options_description& cfg ) override;
      void plugin_initialize( const variables_map& options );
      void plugin_startup();
      void plugin_shutdown();
      #ifdef IS_TEST_NET
      std::map< uint32_t, signed_block > get_map();
      #endif
      std::unique_ptr< detail::fork_monitor_plugin_impl > _my;


};
} } }

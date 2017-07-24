#pragma once
#include <steemit/plugins/follow/follow_operations.hpp>

#include <steemit/plugins/chain/chain_plugin.hpp>

#include <steemit/chain/generic_custom_operation_interpreter.hpp>


#define FOLLOW_PLUGIN_NAME "follow"


namespace steemit { namespace plugins{ namespace follow {

using namespace appbase;
using steemit::chain::generic_custom_operation_interpreter;

class follow_plugin : public appbase::plugin< follow_plugin >
{
   public:
      follow_plugin();
      virtual ~follow_plugin();

      APPBASE_PLUGIN_REQUIRES( (steemit::plugins::chain::chain_plugin) )

      void set_program_options(
         options_description& cli,
         options_description& cfg );
      void plugin_initialize( const variables_map& options );
      void plugin_startup();
      void plugin_shutdown();

      uint32_t max_feed_size = 500;
      fc::time_point_sec start_feeds;

      std::shared_ptr< generic_custom_operation_interpreter< follow_plugin_operation > > _custom_operation_interpreter;
      std::unique_ptr< class follow_plugin_impl > _my;
};

} } } //steemit::follow

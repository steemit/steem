#include <dpn/plugins/network_broadcast_api/network_broadcast_api_plugin.hpp>
#include <dpn/plugins/network_broadcast_api/network_broadcast_api.hpp>

namespace dpn { namespace plugins { namespace network_broadcast_api {

network_broadcast_api_plugin::network_broadcast_api_plugin() {}
network_broadcast_api_plugin::~network_broadcast_api_plugin() {}

void network_broadcast_api_plugin::set_program_options( options_description& cli, options_description& cfg ) {}

void network_broadcast_api_plugin::plugin_initialize( const variables_map& options )
{
   api = std::make_shared< network_broadcast_api >();
   FC_ASSERT( !appbase::app().get_plugin< rc::rc_plugin >().get_rc_plugin_skip_flags().skip_reject_not_enough_rc,
      "rc-skip-reject-not-enough-rc=false is required to broadcast transactions" );
}

void network_broadcast_api_plugin::plugin_startup() {}
void network_broadcast_api_plugin::plugin_shutdown() {}

} } } // dpn::plugins::test_api

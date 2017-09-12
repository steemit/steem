#include <steem/plugins/network_broadcast_api/network_broadcast_api_plugin.hpp>
#include <steem/plugins/network_broadcast_api/network_broadcast_api.hpp>

namespace steem { namespace plugins { namespace network_broadcast_api {

network_broadcast_api_plugin::network_broadcast_api_plugin() {}
network_broadcast_api_plugin::~network_broadcast_api_plugin() {}

void network_broadcast_api_plugin::set_program_options( options_description& cli, options_description& cfg ) {}

void network_broadcast_api_plugin::plugin_initialize( const variables_map& options )
{
   api = std::make_shared< network_broadcast_api >();
}

void network_broadcast_api_plugin::plugin_startup() {}
void network_broadcast_api_plugin::plugin_shutdown() {}

} } } // steem::plugins::test_api

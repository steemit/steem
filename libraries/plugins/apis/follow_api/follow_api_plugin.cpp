#include <steemit/plugins/follow_api/follow_api_plugin.hpp>
#include <steemit/plugins/follow_api/follow_api.hpp>


namespace steemit { namespace plugins { namespace follow {

follow_api_plugin::follow_api_plugin() {}
follow_api_plugin::~follow_api_plugin() {}

void follow_api_plugin::set_program_options( options_description& cli, options_description& cfg ) {}

void follow_api_plugin::plugin_initialize( const variables_map& options )
{
   api = std::make_shared< follow_api >();
}

void follow_api_plugin::plugin_startup() {}
void follow_api_plugin::plugin_shutdown() {}

} } } // steemit::plugins::follow

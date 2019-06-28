#include <dpn/plugins/rewards_api/rewards_api_plugin.hpp>
#include <dpn/plugins/rewards_api/rewards_api.hpp>

namespace dpn { namespace plugins { namespace rewards_api {

rewards_api_plugin::rewards_api_plugin() {}
rewards_api_plugin::~rewards_api_plugin() {}

void rewards_api_plugin::set_program_options( boost::program_options::options_description& cli, boost::program_options::options_description& cfg ) {}

void rewards_api_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   api = std::make_unique< rewards_api >();
}

void rewards_api_plugin::plugin_startup()
{
   elog( "NOTIFYALERT! ${name} is for testing purposes only, do not run in production", ("name", name()) );
}

void rewards_api_plugin::plugin_shutdown() {}

} } } // dpn::plugins::rewards_api


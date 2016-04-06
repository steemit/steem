
#include <steemit/app/application.hpp>
#include <steemit/plugin/core_info/core_info_api.hpp>
#include <steemit/plugin/core_info/core_info_plugin.hpp>

namespace steemit { namespace plugin { namespace core_info {

core_info_plugin::core_info_plugin() {}
core_info_plugin::~core_info_plugin() {}

std::string core_info_plugin::plugin_name()const
{
   return "core_info_plugn";
}

void core_info_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{

}

void core_info_plugin::plugin_startup()
{
   app().register_api_factory< core_info_api >( "core_info_api" );
}

} } }

STEEMIT_DEFINE_PLUGIN( core_info, steemit::plugin::core_info::core_info_plugin )

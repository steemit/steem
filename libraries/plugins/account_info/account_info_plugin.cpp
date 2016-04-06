
#include <steemit/app/application.hpp>
#include <steemit/plugin/account_info/account_info_api.hpp>
#include <steemit/plugin/account_info/account_info_plugin.hpp>

namespace steemit { namespace plugin { namespace account_info {

account_info_plugin::account_info_plugin() {}
account_info_plugin::~account_info_plugin() {}

std::string account_info_plugin::plugin_name()const
{
   return "account_info_plugn";
}

void account_info_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{

}

void account_info_plugin::plugin_startup()
{
   app().register_api_factory< account_info_api >( "account_info_api" );
}

} } }

STEEMIT_DEFINE_PLUGIN( account_info, steemit::plugin::account_info::account_info_plugin )

#include <steemit/plugins/market_history_api/market_history_api_plugin.hpp>
#include <steemit/plugins/market_history_api/market_history_api.hpp>


namespace steemit { namespace plugins { namespace market_history {

market_history_api_plugin::market_history_api_plugin() : appbase::plugin< market_history_api_plugin >( STEEM_MARKET_HISTORY_API_PLUGIN_NAME ) {}
market_history_api_plugin::~market_history_api_plugin() {}

void market_history_api_plugin::set_program_options( options_description& cli, options_description& cfg ) {}

void market_history_api_plugin::plugin_initialize( const variables_map& options )
{
   api = std::make_shared< market_history_api >();
}

void market_history_api_plugin::plugin_startup() {}
void market_history_api_plugin::plugin_shutdown() {}

} } } // steemit::plugins::market_history

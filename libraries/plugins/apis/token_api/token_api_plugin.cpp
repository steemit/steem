#include <steem/plugins/token_api/token_api.hpp>
#include <steem/plugins/token_api/token_api_plugin.hpp>

namespace steem { namespace plugins { namespace token_api {

token_api_plugin::token_api_plugin() {}
token_api_plugin::~token_api_plugin() {}

void token_api_plugin::set_program_options(
   options_description& cli,
   options_description& cfg ) {}

void token_api_plugin::plugin_initialize( const variables_map& options )
{
   api = std::make_shared< token_api >();
}

void token_api_plugin::plugin_startup() {}

void token_api_plugin::plugin_shutdown() {}

} } } // steem::plugins::token_api

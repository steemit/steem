#include <steem/plugins/test_api/test_api_plugin.hpp>

#include <fc/log/logger_config.hpp>

namespace steem { namespace plugins { namespace test_api {

test_api_plugin::test_api_plugin() {}
test_api_plugin::~test_api_plugin() {}

void test_api_plugin::plugin_initialize( const variables_map& options )
{
   JSON_RPC_REGISTER_API( name() );
}

void test_api_plugin::plugin_startup() {}
void test_api_plugin::plugin_shutdown() {}

} } } // steem::plugins::test_api

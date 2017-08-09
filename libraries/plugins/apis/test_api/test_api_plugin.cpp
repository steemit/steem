#include <steemit/plugins/test_api/test_api_plugin.hpp>

#include <fc/log/logger_config.hpp>

namespace steemit { namespace plugins { namespace test_api {

test_api_plugin::test_api_plugin() : appbase::plugin< test_api_plugin >( STEEM_TEST_API_PLUGIN_NAME ) {}
test_api_plugin::~test_api_plugin() {}

void test_api_plugin::plugin_initialize( const variables_map& options )
{
   JSON_RPC_REGISTER_API( name(), (test_api_a)(test_api_b) );
}

void test_api_plugin::plugin_startup() {}
void test_api_plugin::plugin_shutdown() {}

} } } // steemit::plugins::test_api
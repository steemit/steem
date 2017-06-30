#include <steemit/plugins/test_api/test_api_plugin.hpp>

#include <fc/log/logger_config.hpp>

namespace steemit { namespace plugins { namespace test_api {

test_api_plugin::test_api_plugin() : appbase::plugin< test_api_plugin >( TEST_API_PLUGIN_NAME ) {}
test_api_plugin::~test_api_plugin() {}

void test_api_plugin::plugin_initialize( const variables_map& options )
{
   appbase::app().get_plugin< plugins::api_register::api_register_plugin >().add_api(
      name(),
      {
         API_METHOD( test_api_a ),
         API_METHOD( test_api_b )
      });
}

void test_api_plugin::plugin_startup() {}
void test_api_plugin::plugin_shutdown() {}

} } } // steemit::plugins::test_api
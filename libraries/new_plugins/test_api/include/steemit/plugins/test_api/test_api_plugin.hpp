#pragma once
#include <appbase/application.hpp>

#include <steemit/plugins/api_register/api_register_plugin.hpp>

#define TEST_API_PLUGIN_NAME "test_api"

#define REGISTER_API( api_name, ... )

namespace steemit { namespace plugins { namespace test_api {

using namespace appbase;

struct test_api_a_args {};
struct test_api_b_args {};

class test_api_plugin : public appbase::plugin< test_api_plugin >
{
   public:
      test_api_plugin();
      virtual ~test_api_plugin();

      //APPBASE_PLUGIN_REQUIRES()
      APPBASE_PLUGIN_REQUIRES( (plugins::api_register::api_register_plugin) );
      virtual void set_program_options( options_description&, options_description& ) override {}

      void plugin_initialize( const variables_map& options );
      void plugin_startup();
      void plugin_shutdown();

      string test_api_a( test_api_a_args args ) { return "A"; }
      string test_api_b( test_api_b_args args ) { return "B"; }
};

} } } // steemit::plugins::api_register

FC_REFLECT( steemit::plugins::test_api::test_api_a_args, )
FC_REFLECT( steemit::plugins::test_api::test_api_b_args, )

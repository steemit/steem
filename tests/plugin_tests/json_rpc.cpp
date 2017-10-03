#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <steem/chain/account_object.hpp>
#include <steem/chain/comment_object.hpp>
#include <steem/protocol/steem_operations.hpp>

#include "../db_fixture/database_fixture.hpp"

using namespace steem::chain;
using namespace steem::protocol;

BOOST_FIXTURE_TEST_SUITE( json_rpc, json_rpc_database_fixture )

BOOST_AUTO_TEST_CASE( basic_validation )
{
   try
   {
      int initial_argc = boost::unit_test::framework::master_test_suite().argc;
      char** initial_argv = boost::unit_test::framework::master_test_suite().argv;

      launch_server( initial_argc, initial_argv );

      std::string request;

      /*
      According to:
         http://www.jsonrpc.org/specification

      -32700 	         Parse error 	   Invalid JSON was received by the server.
      -32600 	         Invalid Request 	The JSON sent is not a valid Request object.
      -32601 	         Method not found 	The method does not exist / is not available.
      -32602 	         Invalid params 	Invalid method parameter(s).
      -32603 	         Internal error 	Internal JSON-RPC error.
      -32000 to -32099 	Server error 	   Reserved for implementation-defined server-errors.
      */

      //==============jsonrpc==============
      request = "{}";
      make_request( request, -32600 );

      request = "[]";
      make_request( request, -32000 );

      request = "[1,2,3]";
      make_array_request( request, -32602 );

      request = "{\"JSONRPC\": \"2.0\", \"method\": \"call\", \"params\": [], \"id\": 1}";
      make_request( request, -32600 );

      request = "{\"jsonrpc\": \"1.0\", \"method\": \"call\", \"params\": [], \"id\": 1}";
      make_request( request, -32600 );

      request = "{\"jsonrpc_\": \"2.0\", \"method\": \"call\", \"params\": [], \"id\": 1}";
      make_request( request, -32600 );

      request = "{\"jsonrpc\": \"2\", \"method\": \"call\", \"params\": [], \"id\": 1}";
      make_request( request, -32600 );

      request = "{\"jsonrpc\": 2.0, \"method\": \"call\", \"params\": [], \"id\": 1}";
      make_request( request, -32600 );

      request = "{\"jsonrpc\": true, \"method\": \"call\", \"params\": [], \"id\": 1}";
      make_request( request, -32600 );

      request = "{\"jsonrpc\": null, \"method\": \"call\", \"params\": [], \"id\": 1}";
      make_request( request, -32600 );

      request = "{\"jsonrpc\": {}, \"method\": \"call\", \"params\": [], \"id\": 1}";
      make_request( request, -32602 );

      request = "{\"jsonrpc\": [], \"method\": \"call\", \"params\": [], \"id\": 1}";
      make_request( request, -32602 );

      request = "{\"jsonrpc\": { \"jsonrpc\":\"2.0\" }, \"method\": \"call\", \"params\": [], \"id\": 1}";
      make_request( request, -32602 );

      request = "\"jsonrpc\" \"2.0\"";
      make_request( request, -32602 );
      //==============jsonrpc==============

      //==============method==============
      request = "{\"jsonrpc\": \"2.0\", \"METHOD\": \"call\", \"params\": [], \"id\": 1}";
      make_request( request, -32600 );

      request = "{\"jsonrpc\": \"2.0\", \"method_\": \"call\", \"params\": [], \"id\": 1}";
      make_request( request, -32600 );

      request = "{\"jsonrpc\": \"2.0\", \"method\": \"xyz\", \"params\": [], \"id\": 1}";
      make_request( request, -32601 );

      request = "{\"jsonrpc\": \"2.0\", \"method\": 123, \"params\": [], \"id\": 1}";
      make_request( request, -32601 );

      request = "{\"jsonrpc\": \"2.0\", \"method\": false, \"params\": [], \"id\": 1}";
      make_request( request, -32601 );

      request = "{\"jsonrpc\": \"2.0\", \"method\": null, \"params\": [], \"id\": 1}";
      make_request( request, -32601 );

      request = "{\"jsonrpc\": \"2.0\", \"method\": {}, \"params\": [], \"id\": 1}";
      make_request( request, -32602 );

      request = "{\"jsonrpc\": \"2.0\", \"method\": [], \"params\": [], \"id\": 1}";
      make_request( request, -32602 );
      //==============method==============

      //==============params==============
      request = "{\"jsonrpc\": \"2.0\", \"method\": \"call\" }";
      make_request( request, -32001, true/*warning*/ );

      request = "{\"jsonrpc\": \"2.0\", \"method\": call, \"params\":1, \"id\": 1}";
      make_request( request, -32000 );

      request = "{\"jsonrpc\": \"2.0\", \"method\": call, \"params\":\"abc\", \"id\": 1}";
      make_request( request, -32000 );

      request = "{\"jsonrpc\": \"2.0\", \"method\": call, \"params\":true, \"id\": 1}";
      make_request( request, -32000 );

      request = "{\"jsonrpc\": \"2.0\", \"method\": call, \"params\":\"true, \"id\": 1}";
      make_request( request, -32000 );

      request = "{\"jsonrpc\": \"2.0\", \"method\": call, \"params\":\"true\", \"id\": 1}";
      make_request( request, -32000 );
      //==============params==============

      //==============id==============
      request = "{\"jsonrpc\": \"2.0\", \"method\": \"call\", \"params\": [], \"id\": null}";
      make_request( request, -32600 );

      request = "{\"jsonrpc\": \"2.0\", \"method\": \"call\", \"params\": [], \"id\": 5.4}";
      make_request( request, -32600 );
      //==============id==============
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( syntax_validation )
{
   try
   {
      int initial_argc = boost::unit_test::framework::master_test_suite().argc;
      char** initial_argv = boost::unit_test::framework::master_test_suite().argv;

      launch_server( initial_argc, initial_argv );

      fc::usleep( fc::seconds(2) );

      std::string request;

      request = "";
      make_request( request, -32000 );

      request = "\"";
      make_request( request, -32000 );

      request = "{";
      make_request( request, -32000 );

      request = "{abcde}";
      make_request( request, -32000 );

      request = "{ \"abcde\" }";
      make_request( request, -32000 );

      request = "{\"jsonrpc\":, \"method\": \"call\", \"params\": [], \"id\": 1}";
      make_request( request, -32000 );

      request = "[{]";
      make_request( request, -32000 );

      request = "[{ \"jsonrpc\": \"2.0\" },{ \"jsonrpc\" }]";
      make_request( request, -32000 );

      request = "{\"jsonrpc\": \"2.0\" \"method\" \"call\"}";
      make_request( request, -32000 );

      request = "{\"jsonrpc\": \"2.0\", \"method\": \"call}";
      make_request( request, -32000 );

      request = "{\"jsonrpc\": \"2.0\", method: call}";
      make_request( request, -32000 );

      request = "[{\"jsonrpc\": \"2.0\", \"method\": \"call\"}";
      make_request( request, -32000 );

      request = "{\"jsonrpc\": \"2.0\", \"method\": \"call\", \"params\": ]}";
      make_request( request, -32000 );

      request = "{\"jsonrpc\": \"2.0\", \"method\": \"call\", \"params\" []}";
      make_request( request, -32000 );

      request = "{\"jsonrpc\": \"2.0\", \"method\": \"call\", \"params\" [5,]}";
      make_request( request, -32000 );

      request = "{\"jsonrpc\": \"2.0\", \"method\": \"call\", \"params\":{\"arg1\"}}";
      make_request( request, -32000 );

      request = "{\"jsonrpc\": \"2.0\", \"method\": \"call\", \"params\":{\"arg1\":}}";
      make_request( request, -32000 );

      request = "[ {\"jsonrpc\": \"2.0\", \"method\": \"call\", \"params\":{}, ]";
      make_request( request, -32000 );

      request = "[ {\"jsonrpc\": \"2.0\", \"method\": \"call\", \"params\":{} }, {\"jsonrpc\": \"2.0\", \"method\" ]";
      make_request( request, -32000 );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( misc_validation )
{
   try
   {
      int initial_argc = boost::unit_test::framework::master_test_suite().argc;
      char** initial_argv = boost::unit_test::framework::master_test_suite().argv;

      launch_server( initial_argc, initial_argv );

      std::string request;

      request = "{\"jsonrpc\": \"2.0\", \"method\": \"a.b.c\", \"params\": [\"a\",\"b\", {} ], \"id\": 1}";
      make_request( request, -32601 );

      request = "{\"jsonrpc\": \"2.0\", \"method\": \"a..c\", \"params\": [\"a\",\"b\", {} ], \"id\": 1}";
      make_request( request, -32601 );

      request = "{\"jsonrpc\": \"2.0\", \"method\": \"fake_api.fake_method\", \"params\": [\"a\",\"b\", {} ], \"id\": 1}";
      make_request( request, -32601 );

      request = "{\"jsonrpc\": \"2.0\", \"method\": \"call\", \"params\": [\"fake_api\",\"fake_method\", {} ], \"id\": 1}";
      make_request( request, -32601 );

      request = "{\"jsonrpc\": \"2.0\", \"method\": \"call\", \"params\": {}, \"id\": 1}";
      make_request( request, -32601 );

      request = "{\"jsonrpc\": \"2.0\", \"method\": \"call\", \"params\": { \"fake_api\":\"database_api\", \"fake_method\":\"get_dynamic_global_properties\", \"fake_args\":{} }, \"id\": 1}";
      make_request( request, -32601 );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()   
#endif

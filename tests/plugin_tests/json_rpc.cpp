#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <steem/chain/account_object.hpp>
#include <steem/chain/comment_object.hpp>
#include <steem/protocol/steem_operations.hpp>
#include <steem/plugins/json_rpc/json_rpc_plugin.hpp>

#include "../db_fixture/database_fixture.hpp"

using namespace steem::chain;
using namespace steem::protocol;

BOOST_FIXTURE_TEST_SUITE( json_rpc, json_rpc_database_fixture )

BOOST_AUTO_TEST_CASE( basic_validation )
{
   try
   {
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
      make_request( request, JSON_RPC_INVALID_REQUEST );

      request = "[]";
      make_request( request, JSON_RPC_SERVER_ERROR );

      request = "[1,2,3]";
      make_array_request( request, JSON_RPC_PARSE_ERROR );

      request = "{\"JSONRPC\": \"2.0\", \"method\": \"call\", \"params\": [], \"id\": 1}";
      make_request( request, JSON_RPC_INVALID_REQUEST );

      request = "{\"jsonrpc\": \"1.0\", \"method\": \"call\", \"params\": [], \"id\": 1}";
      make_request( request, JSON_RPC_INVALID_REQUEST );

      request = "{\"jsonrpc_\": \"2.0\", \"method\": \"call\", \"params\": [], \"id\": 1}";
      make_request( request, JSON_RPC_INVALID_REQUEST );

      request = "{\"jsonrpc\": \"2\", \"method\": \"call\", \"params\": [], \"id\": 1}";
      make_request( request, JSON_RPC_INVALID_REQUEST );

      request = "{\"jsonrpc\": 2.0, \"method\": \"call\", \"params\": [], \"id\": 1}";
      make_request( request, JSON_RPC_INVALID_REQUEST );

      request = "{\"jsonrpc\": true, \"method\": \"call\", \"params\": [], \"id\": 1}";
      make_request( request, JSON_RPC_INVALID_REQUEST );

      request = "{\"jsonrpc\": null, \"method\": \"call\", \"params\": [], \"id\": 1}";
      make_request( request, JSON_RPC_INVALID_REQUEST );

      request = "{\"jsonrpc\": {}, \"method\": \"call\", \"params\": [], \"id\": 1}";
      make_request( request, JSON_RPC_INVALID_REQUEST );

      request = "{\"jsonrpc\": [], \"method\": \"call\", \"params\": [], \"id\": 1}";
      make_request( request, JSON_RPC_INVALID_REQUEST );

      request = "{\"jsonrpc\": { \"jsonrpc\":\"2.0\" }, \"method\": \"call\", \"params\": [], \"id\": 1}";
      make_request( request, JSON_RPC_INVALID_REQUEST );

      request = "\"jsonrpc\" \"2.0\"";
      make_request( request, JSON_RPC_PARSE_ERROR );
      //==============jsonrpc==============

      //==============method==============
      request = "{\"jsonrpc\": \"2.0\", \"METHOD\": \"call\", \"params\": [], \"id\": 1}";
      make_request( request, JSON_RPC_INVALID_REQUEST );

      request = "{\"jsonrpc\": \"2.0\", \"method_\": \"call\", \"params\": [], \"id\": 1}";
      make_request( request, JSON_RPC_INVALID_REQUEST );

      request = "{\"jsonrpc\": \"2.0\", \"method\": \"xyz\", \"params\": [], \"id\": 1}";
      make_request( request, JSON_RPC_PARSE_PARAMS_ERROR );

      request = "{\"jsonrpc\": \"2.0\", \"method\": 123, \"params\": [], \"id\": 1}";
      make_request( request, JSON_RPC_INVALID_REQUEST );

      request = "{\"jsonrpc\": \"2.0\", \"method\": false, \"params\": [], \"id\": 1}";
      make_request( request, JSON_RPC_INVALID_REQUEST );

      request = "{\"jsonrpc\": \"2.0\", \"method\": null, \"params\": [], \"id\": 1}";
      make_request( request, JSON_RPC_INVALID_REQUEST );

      request = "{\"jsonrpc\": \"2.0\", \"method\": {}, \"params\": [], \"id\": 1}";
      make_request( request, JSON_RPC_INVALID_REQUEST );

      request = "{\"jsonrpc\": \"2.0\", \"method\": [], \"params\": [], \"id\": 1}";
      make_request( request, JSON_RPC_INVALID_REQUEST );
      //==============method==============

      //==============params==============
      request = "{\"jsonrpc\": \"2.0\", \"method\": \"call\" }";
      make_request( request, JSON_RPC_NO_PARAMS, true/*warning*/ );

      request = "{\"jsonrpc\": \"2.0\", \"method\": call, \"params\":1, \"id\": 1}";
      make_request( request, JSON_RPC_SERVER_ERROR );

      request = "{\"jsonrpc\": \"2.0\", \"method\": call, \"params\":\"abc\", \"id\": 1}";
      make_request( request, JSON_RPC_SERVER_ERROR );

      request = "{\"jsonrpc\": \"2.0\", \"method\": call, \"params\":true, \"id\": 1}";
      make_request( request, JSON_RPC_SERVER_ERROR );

      request = "{\"jsonrpc\": \"2.0\", \"method\": call, \"params\":\"true, \"id\": 1}";
      make_request( request, JSON_RPC_SERVER_ERROR );

      request = "{\"jsonrpc\": \"2.0\", \"method\": call, \"params\":\"true\", \"id\": 1}";
      make_request( request, JSON_RPC_SERVER_ERROR );
      //==============params==============

      //==============id==============
      request = "{\"jsonrpc\": \"2.0\", \"method\": \"call\", \"params\": [], \"id\": null}";
      make_request( request, JSON_RPC_INVALID_REQUEST );

      request = "{\"jsonrpc\": \"2.0\", \"method\": \"call\", \"params\": [], \"id\": 5.4}";
      make_request( request, JSON_RPC_INVALID_REQUEST );
      //==============id==============
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( syntax_validation )
{
   try
   {
      std::string request;

      request = "";
      make_request( request, JSON_RPC_SERVER_ERROR );

      request = "\"";
      make_request( request, JSON_RPC_SERVER_ERROR );

      request = "{";
      make_request( request, JSON_RPC_SERVER_ERROR );

      request = "{abcde}";
      make_request( request, JSON_RPC_SERVER_ERROR );

      request = "{ \"abcde\" }";
      make_request( request, JSON_RPC_SERVER_ERROR );

      request = "{\"jsonrpc\":, \"method\": \"call\", \"params\": [], \"id\": 1}";
      make_request( request, JSON_RPC_SERVER_ERROR );

      request = "[{]";
      make_request( request, JSON_RPC_SERVER_ERROR );

      request = "[{ \"jsonrpc\": \"2.0\" },{ \"jsonrpc\" }]";
      make_request( request, JSON_RPC_SERVER_ERROR );

      request = "{\"jsonrpc\": \"2.0\" \"method\" \"call\"}";
      make_request( request, JSON_RPC_SERVER_ERROR );

      request = "{\"jsonrpc\": \"2.0\", \"method\": \"call}";
      make_request( request, JSON_RPC_SERVER_ERROR );

      request = "{\"jsonrpc\": \"2.0\", method: call}";
      make_request( request, JSON_RPC_SERVER_ERROR );

      request = "[{\"jsonrpc\": \"2.0\", \"method\": \"call\"}";
      make_request( request, JSON_RPC_SERVER_ERROR );

      request = "{\"jsonrpc\": \"2.0\", \"method\": \"call\", \"params\": ]}";
      make_request( request, JSON_RPC_SERVER_ERROR );

      request = "{\"jsonrpc\": \"2.0\", \"method\": \"call\", \"params\" []}";
      make_request( request, JSON_RPC_SERVER_ERROR );

      request = "{\"jsonrpc\": \"2.0\", \"method\": \"call\", \"params\" [5,]}";
      make_request( request, JSON_RPC_SERVER_ERROR );

      request = "{\"jsonrpc\": \"2.0\", \"method\": \"call\", \"params\":{\"arg1\"}}";
      make_request( request, JSON_RPC_SERVER_ERROR );

      request = "{\"jsonrpc\": \"2.0\", \"method\": \"call\", \"params\":{\"arg1\":}}";
      make_request( request, JSON_RPC_SERVER_ERROR );

      request = "[ {\"jsonrpc\": \"2.0\", \"method\": \"call\", \"params\":{}, ]";
      make_request( request, JSON_RPC_SERVER_ERROR );

      request = "[ {\"jsonrpc\": \"2.0\", \"method\": \"call\", \"params\":{} }, {\"jsonrpc\": \"2.0\", \"method\" ]";
      make_request( request, JSON_RPC_SERVER_ERROR );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( misc_validation )
{
   try
   {
      std::string request;

      request = "{\"jsonrpc\": \"2.0\", \"method\": \"a.b.c\", \"params\": [\"a\",\"b\", {} ], \"id\": 1}";
      make_request( request, JSON_RPC_PARSE_PARAMS_ERROR );

      request = "{\"jsonrpc\": \"2.0\", \"method\": \"a..c\", \"params\": [\"a\",\"b\", {} ], \"id\": 1}";
      make_request( request, JSON_RPC_PARSE_PARAMS_ERROR );

      request = "{\"jsonrpc\": \"2.0\", \"method\": \"fake_api.fake_method\", \"params\": [\"a\",\"b\", {} ], \"id\": 1}";
      make_request( request, JSON_RPC_PARSE_PARAMS_ERROR );

      request = "{\"jsonrpc\": \"2.0\", \"method\": \"call\", \"params\": [\"fake_api\",\"fake_method\", {} ], \"id\": 1}";
      make_request( request, JSON_RPC_PARSE_PARAMS_ERROR );

      request = "{\"jsonrpc\": \"2.0\", \"method\": \"call\", \"params\": {}, \"id\": 1}";
      make_request( request, JSON_RPC_PARSE_PARAMS_ERROR );

      request = "{\"jsonrpc\": \"2.0\", \"method\": \"call\", \"params\": { \"fake_api\":\"database_api\", \"fake_method\":\"get_dynamic_global_properties\", \"fake_args\":{} }, \"id\": 1}";
      make_request( request, JSON_RPC_PARSE_PARAMS_ERROR );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( positive_validation )
{
   try
   {
      std::string request;

      request = "{\"jsonrpc\":\"2.0\", \"method\":\"call\", \"params\":[\"database_api\", \"get_dynamic_global_properties\"], \"id\":1}";
      make_positive_request( request );

      request = "{\"jsonrpc\":\"2.0\", \"method\":\"call\", \"params\":[\"database_api\", \"get_dynamic_global_properties\", {}], \"id\":3}";
      make_positive_request( request );

      request = "{\"jsonrpc\":\"2.0\", \"method\":\"database_api.get_dynamic_global_properties\",\"id\":4}";
      make_positive_request( request );

      request = "{\"jsonrpc\":\"2.0\", \"method\":\"database_api.get_dynamic_global_properties\", \"params\":{}, \"id\":5}";
      make_positive_request( request );

      request = "{\"jsonrpc\":\"2.0\", \"method\":\"call\", \"params\":[\"condenser_api\", \"get_dynamic_global_properties\", []], \"id\":8}";
      make_positive_request( request );

      request = "{\"jsonrpc\":\"2.0\", \"method\":\"condenser_api.get_dynamic_global_properties\", \"params\":[], \"id\":12}";
      make_positive_request( request );

      request = "{\"jsonrpc\":\"2.0\", \"method\":\"call\", \"params\":[\"database_api\", \"find_accounts\", {\"accounts\":[\"init_miner\"]}], \"id\":13}";
      make_positive_request( request );

      request = "{\"jsonrpc\":\"2.0\", \"method\":\"database_api.find_accounts\", \"params\":{\"accounts\":[\"init_miner\"]}, \"id\":14}";
      make_positive_request( request );

      request = "{\"jsonrpc\":\"2.0\", \"method\":\"call\", \"params\":[\"database_api\", \"find_accounts\", {}], \"id\":15}";
      make_positive_request( request );

      request = "{\"jsonrpc\":\"2.0\", \"method\":\"call\", \"params\":[\"database_api\", \"find_accounts\"], \"id\":15}";
      make_positive_request( request );

      request = "{\"jsonrpc\":\"2.0\", \"method\":\"database_api.find_accounts\", \"params\":{\"accounts\":[\"init_miner\"]}, \"id\":17}";
      make_positive_request( request );

      request = "{\"jsonrpc\":\"2.0\", \"method\":\"database_api.find_accounts\", \"params\":{}, \"id\":16}";
      make_positive_request( request );

      request = "{\"jsonrpc\":\"2.0\", \"method\":\"database_api.find_accounts\", \"id\":18}";
      make_positive_request( request );

      request = "{\"jsonrpc\":\"2.0\", \"method\":\"call\", \"params\":[\"condenser_api\", \"get_accounts\", [[\"init_miner\"]]], \"id\":6}";
      make_positive_request( request );

      request = "{\"jsonrpc\":\"2.0\", \"method\":\"condenser_api.get_accounts\", \"params\":[[\"init_miner\"]], \"id\":7}";
      make_positive_request( request );

      request = "{\"jsonrpc\":\"2.0\", \"method\":\"call\", \"params\":[\"condenser_api\", \"get_accounts\", [[]]], \"id\":8}";
      make_positive_request( request );

      request = "{\"jsonrpc\":\"2.0\", \"method\":\"condenser_api.get_accounts\", \"params\":[[]], \"id\":9}";
      make_positive_request( request );

      request = "{\"jsonrpc\": \"2.0\", \"method\": \"call\", \"params\": [\"block_api\",\"get_block\", {\"block_num\":23} ], \"id\": 10}";
      make_positive_request( request );

      request = "{\"jsonrpc\": \"2.0\", \"method\": \"block_api.get_block\", \"params\": {\"block_num\":0}, \"id\": 11}";
      make_positive_request( request );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( semantics_validation )
{
   try
   {
      std::string request;

      request = "{\"jsonrpc\":\"2.0\", \"method\":\"call\", \"params\":[\"database_api\", \"get_dynamic_global_properties\"], \"id\":20 }";
      make_positive_request( request );

      request = "{\"jsonrpc\":\"2.0\", \"method\":\"call\", \"params\":[\"database_api\", \"get_dynamic_global_properties\"], \"id\":\"20\" }";
      make_positive_request( request );

      request = "{\"jsonrpc\":\"2.0\", \"method\":\"call\", \"params\":[\"database_api\", \"get_dynamic_global_properties\"], \"id\":-20 }";
      make_positive_request( request );

      request = "{\"jsonrpc\":\"2.0\", \"method\":\"call\", \"params\":[\"database_api\", \"get_dynamic_global_properties\"], \"id\":\"-20\" }";
      make_positive_request( request );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
#endif

#include <steemit/plugins/json_rpc/json_rpc_plugin.hpp>

#include <boost/algorithm/string.hpp>

#include <fc/log/logger_config.hpp>
#include <fc/exception/exception.hpp>

#define JSON_RPC_PARSE_ERROR        (-32700)
#define JSON_RPC_INVALID_REQUEST    (-32600)
#define JSON_RPC_METHOD_NOT_FOUND   (-32601)
#define JSON_RPC_INVALID_PARAMS     (-32602)
#define JSON_RPC_INTERNAL_ERROR     (-32603)
#define JSON_RPC_SERVER_ERROR       (-32000)

namespace steemit { namespace plugins { namespace json_rpc {

using detail::json_rpc_error;
using detail::json_rpc_response;

json_rpc_plugin::json_rpc_plugin() : appbase::plugin< json_rpc_plugin >( JSON_RPC_PLUGIN_NAME ) {}
json_rpc_plugin::~json_rpc_plugin() {}

void json_rpc_plugin::plugin_initialize( const variables_map& options ) {}
void json_rpc_plugin::plugin_startup() {}
void json_rpc_plugin::plugin_shutdown() {}

string json_rpc_plugin::call( const string& message )
{
   wdump( (message) );
   json_rpc_response response;

   try
   {
      const auto request = fc::json::from_string( message ).get_object();

      if( request.contains( "id" ) )
      {
         try
         {
            response.id = request[ "id" ].as_int64();
         }
         catch( fc::exception& )
         {
            try
            {
               response.id = request[ "id" ].as_string();
            }
            catch( fc::exception& ) {}
         }
      }

      if( request.contains( "jsonrpc" ) && request[ "jsonrpc" ].as_string() == "2.0" )
      {
         if( request.contains( "method" ) )
         {
            try
            {
               string method = request[ "method" ].as_string();

               detail::api_call* call;
               fc::variant params;

               // This is to maintain backwards compatibility with existing call structure.
               if( method == "call" )
               {
                  std::vector< fc::variant > v = request[ "params" ].as< std::vector< fc::variant > >();
                  FC_ASSERT( v.size() == 2 || v.size() == 3, "params should be {\"api\", \"method\", \"args\"" );

                  auto api_itr = _registered_apis.find( v[0].as_string() );
                  FC_ASSERT( api_itr != _registered_apis.end(), "Could not find API ${api}", ("api", v[0]) );

                  auto method_itr = api_itr->second.find( v[1].as_string() );
                  FC_ASSERT( method_itr != api_itr->second.end(), "Could not find method ${method}", ("method", v[1]) );

                  call = &(method_itr->second);

                  if( v.size() == 3 )
                     params = v[2];
                  else
                     params = fc::json::from_string( "{}" );
               }
               else
               {
                  vector< std::string > v;
                  boost::split( v, method, boost::is_any_of( "." ) );

                  FC_ASSERT( v.size() == 2, "method specification invalid. Should be api.method" );

                  auto api_itr = _registered_apis.find( v[0] );
                  FC_ASSERT( api_itr != _registered_apis.end(), "Could not find API ${api}", ("api", v[0]) );

                  auto method_itr = api_itr->second.find( v[1] );
                  FC_ASSERT( method_itr != api_itr->second.end(), "Could not find method ${method}", ("method", v[1]) );

                  call = &(method_itr->second);

                  try
                  {
                     params = request.contains( "params" ) ? request[ "params" ] : fc::json::from_string( "{}" );
                  }
                  catch( fc::parse_error_exception& e )
                  {
                     response.error = json_rpc_error( JSON_RPC_INVALID_PARAMS, e.to_string(), fc::json::to_string( *(e.dynamic_copy_exception()) ) );
                  }
                  catch( fc::bad_cast_exception& e )
                  {
                     response.error = json_rpc_error( JSON_RPC_INVALID_PARAMS, e.to_string(), fc::json::to_string( *(e.dynamic_copy_exception()) ) );
                  }
               }

               try
               {
                  response.result = (*call)( params );
               }
               catch( fc::exception& e )
               {
                  response.error = json_rpc_error( JSON_RPC_SERVER_ERROR, e.to_string(), fc::json::to_string( *(e.dynamic_copy_exception()) ) );
               }

            }
            catch( fc::assert_exception& e )
            {
               response.error = json_rpc_error( JSON_RPC_METHOD_NOT_FOUND, e.to_string(), fc::json::to_string( *(e.dynamic_copy_exception()) ) );
            }
         }
      }
      else
      {
         response.error = json_rpc_error( JSON_RPC_INVALID_REQUEST, "jsonrpc value is not \"2.0\"" );
      }
   }
   catch( fc::parse_error_exception& e )
   {
      response.error = json_rpc_error( JSON_RPC_INVALID_PARAMS, e.to_string(), fc::json::to_string( *(e.dynamic_copy_exception()) ) );
   }
   catch( fc::bad_cast_exception& e )
   {
      response.error = json_rpc_error( JSON_RPC_INVALID_PARAMS, e.to_string(), fc::json::to_string( *(e.dynamic_copy_exception()) ) );
   }

   return fc::json::to_string( response );
}

} } } // steemit::plugins::json_rpc
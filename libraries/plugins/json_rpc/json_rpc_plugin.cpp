#include <steem/plugins/json_rpc/json_rpc_plugin.hpp>

#include <boost/algorithm/string.hpp>

#include <fc/log/logger_config.hpp>
#include <fc/exception/exception.hpp>

#define JSON_RPC_PARSE_ERROR        (-32700)
#define JSON_RPC_INVALID_REQUEST    (-32600)
#define JSON_RPC_METHOD_NOT_FOUND   (-32601)
#define JSON_RPC_INVALID_PARAMS     (-32602)
#define JSON_RPC_INTERNAL_ERROR     (-32603)
#define JSON_RPC_SERVER_ERROR       (-32000)
#define JSON_RPC_NO_PARAMS          (-32001)

namespace steem { namespace plugins { namespace json_rpc {

namespace detail
{
   struct json_rpc_error
   {
      json_rpc_error()
         : code( 0 ) {}

      json_rpc_error( int32_t c, std::string m, fc::optional< fc::variant > d = fc::optional< fc::variant >() )
         : code( c ), message( m ), data( d ) {}

      int32_t                          code;
      std::string                      message;
      fc::optional< fc::variant >      data;
   };

   struct json_rpc_response
   {
      std::string                      jsonrpc = "2.0";
      fc::optional< fc::variant >      result;
      fc::optional< json_rpc_error >   error;
      fc::variant                      id;
   };

   class json_rpc_plugin_impl
   {
      public:
         json_rpc_plugin_impl();
         ~json_rpc_plugin_impl();

         void add_api_method( const string& api_name, const string& method_name, const api_method& api );

         api_method* find_api_method( std::string api, std::string method );
         api_method* process_params( string method, const fc::variant_object& request, fc::variant& func_args );
         void rpc_id( const fc::variant_object& request, json_rpc_response& response );
         void rpc_jsonrpc( const fc::variant_object& request, json_rpc_response& response );
         json_rpc_response rpc( const fc::variant& message );

         map< string, api_description > _registered_apis;
   };

   json_rpc_plugin_impl::json_rpc_plugin_impl() {}
   json_rpc_plugin_impl::~json_rpc_plugin_impl() {}

   void json_rpc_plugin_impl::add_api_method( const string& api_name, const string& method_name, const api_method& api )
   {
      _registered_apis[ api_name ][ method_name ] = api;
   }

   api_method* json_rpc_plugin_impl::find_api_method( std::string api, std::string method )
   {
      auto api_itr = _registered_apis.find( api );
      FC_ASSERT( api_itr != _registered_apis.end(), "Could not find API ${api}", ("api", api) );

      auto method_itr = api_itr->second.find( method );
      FC_ASSERT( method_itr != api_itr->second.end(), "Could not find method ${method}", ("method", method) );

      return &(method_itr->second);
   }

   api_method* json_rpc_plugin_impl::process_params( string method, const fc::variant_object& request, fc::variant& func_args )
   {
      api_method* ret = nullptr;
      bool call_mode = method == "call";

      FC_ASSERT( ( call_mode && request.contains( "params" ) ) || !call_mode );

      if( call_mode )
      {
         std::vector< fc::variant > v;

         if( request[ "params" ].is_array() )
            v = request[ "params" ].as< std::vector< fc::variant > >();
         else if( request[ "params" ].is_object() )
         {
            fc::variant_object _params = request[ "params" ].get_object();
            if( _params.contains("api") )
               v.push_back( _params["api"] );
            if( _params.contains("method") )
               v.push_back( _params["method"] );
            if( _params.contains("args") )
               v.push_back( _params["args"] );
         }

         FC_ASSERT( v.size() == 2 || v.size() == 3, "params should be {\"api\", \"method\", \"args\"" );

         ret = find_api_method( v[0].as_string(), v[1].as_string() );

         func_args = ( v.size() == 3 )?v[2]:fc::json::from_string( "{}" );
      }
      else
      {
         vector< std::string > v;
         boost::split( v, method, boost::is_any_of( "." ) );

         FC_ASSERT( v.size() == 2, "method specification invalid. Should be api.method" );

         ret = find_api_method( v[0], v[1] );

         func_args = request.contains( "params" ) ? request[ "params" ] : fc::json::from_string( "{}" );
      }

      //Switch empty array to empty object, i.e. : [] -> {}
      if( func_args.is_array() )
      {
         size_t _size = func_args.as< std::vector< fc::variant > >().size();
         if( _size == 0 )
            func_args = fc::json::from_string( "{}" );
      }

#ifdef IS_TEST_NET
      std::string str_func_params = fc::json::to_string( func_args );
#endif

      return ret;
   }

   void json_rpc_plugin_impl::rpc_id( const fc::variant_object& request, json_rpc_response& response )
   {
      if( request.contains( "id" ) )
      {
         if( request[ "id" ].is_null() || request[ "id" ].is_double() )
            response.error = json_rpc_error( JSON_RPC_INVALID_REQUEST, "Only integer value or string is allowed for member \"id\"" );
         else
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
               catch( fc::exception& )
               {
                  response.error = json_rpc_error( JSON_RPC_INVALID_REQUEST, "Only integer value or string is allowed for member \"id\"" );
               }
            }
            catch(...)
            {
                  response.error = json_rpc_error( JSON_RPC_INVALID_REQUEST, "Only integer value or string is allowed for member \"id\"" );
            }
         }
      }
   }

   void json_rpc_plugin_impl::rpc_jsonrpc( const fc::variant_object& request, json_rpc_response& response )
   {
      if( request.contains( "jsonrpc" ) && request[ "jsonrpc" ].as_string() == "2.0" )
      {
         if( request.contains( "method" ) )
         {
            try
            {
               string method = request[ "method" ].as_string();

               // This is to maintain backwards compatibility with existing call structure.
               if( ( method == "call" && request.contains( "params" ) ) || method != "call" )
               {
                  fc::variant func_args;

                  api_method* call = process_params( method, request, func_args );
                  if( !call )
                     FC_THROW( "Api method is null" );
                  response.result = (*call)( func_args );
               }
               else
               {
                  response.error = json_rpc_error( JSON_RPC_NO_PARAMS, "A member \"params\" does not exist" );
               }
            }
            catch( fc::assert_exception& e )
            {
               response.error = json_rpc_error( JSON_RPC_METHOD_NOT_FOUND, e.to_string(), fc::variant( *(e.dynamic_copy_exception()) ) );
            }
         }
         else
         {
            response.error = json_rpc_error( JSON_RPC_INVALID_REQUEST, "A member \"method\" does not exist" );
         }
      }
      else
      {
         response.error = json_rpc_error( JSON_RPC_INVALID_REQUEST, "jsonrpc value is not \"2.0\"" );
      }
   }

   json_rpc_response json_rpc_plugin_impl::rpc( const fc::variant& message )
   {
      json_rpc_response response;

      ddump( (message) );

      try
      {
         const auto request = message.get_object();

         rpc_id( request, response );
         if( !response.error.valid() )
            rpc_jsonrpc( request, response );
      }
      catch( fc::parse_error_exception& e )
      {
         response.error = json_rpc_error( JSON_RPC_INVALID_PARAMS, e.to_string(), fc::variant( *(e.dynamic_copy_exception()) ) );
      }
      catch( fc::bad_cast_exception& e )
      {
         response.error = json_rpc_error( JSON_RPC_INVALID_PARAMS, e.to_string(), fc::variant( *(e.dynamic_copy_exception()) ) );
      }
      catch( fc::exception& e )
      {
         response.error = json_rpc_error( JSON_RPC_SERVER_ERROR, e.to_string(), fc::variant( *(e.dynamic_copy_exception()) ) );
      }
      catch( ... )
      {
         response.error = json_rpc_error( JSON_RPC_SERVER_ERROR, "Unknown error - parsing rpc message failed" );
      }

      return response;
   }
}

using detail::json_rpc_error;
using detail::json_rpc_response;

json_rpc_plugin::json_rpc_plugin() : my( new detail::json_rpc_plugin_impl() ) {}
json_rpc_plugin::~json_rpc_plugin() {}

void json_rpc_plugin::plugin_initialize( const variables_map& options ) {}
void json_rpc_plugin::plugin_startup() {}
void json_rpc_plugin::plugin_shutdown() {}

void json_rpc_plugin::add_api_method( const string& api_name, const string& method_name, const api_method& api )
{
   my->add_api_method( api_name, method_name, api );
}

string json_rpc_plugin::call( const string& message )
{
   try
   {
      fc::variant v = fc::json::from_string( message );

      if( v.is_array() )
      {
         vector< fc::variant > messages = v.as< vector< fc::variant > >();
         vector< json_rpc_response > responses;

         if( messages.size() )
         {
            responses.reserve( messages.size() );

            for( auto& m : messages )
               responses.push_back( my->rpc( m ) );

            return fc::json::to_string( responses );
         }
         else
         {
            //For example: message == "[]"
            json_rpc_response response;
            response.error = json_rpc_error( JSON_RPC_SERVER_ERROR, "Array is invalid" );
            return fc::json::to_string( response );
         }
      }
      else
      {
         return fc::json::to_string( my->rpc( v ) );
      }
   }
   catch( fc::exception& e )
   {
      json_rpc_response response;
      response.error = json_rpc_error( JSON_RPC_SERVER_ERROR, e.to_string(), fc::variant( *(e.dynamic_copy_exception()) ) );
      return fc::json::to_string( response );
   }

}

} } } // steem::plugins::json_rpc

FC_REFLECT( steem::plugins::json_rpc::detail::json_rpc_error, (code)(message)(data) )
FC_REFLECT( steem::plugins::json_rpc::detail::json_rpc_response, (jsonrpc)(result)(error)(id) )

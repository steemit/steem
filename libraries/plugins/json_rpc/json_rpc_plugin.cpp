#include <steem/plugins/json_rpc/json_rpc_plugin.hpp>
#include <steem/plugins/json_rpc/utility.hpp>

#include <steem/plugins/statsd/utility.hpp>

#include <boost/algorithm/string.hpp>

#include <fc/log/logger_config.hpp>
#include <fc/exception/exception.hpp>
#include <fc/macros.hpp>
#include <fc/io/fstream.hpp>

#include <chainbase/chainbase.hpp>

#define ENABLE_JSON_RPC_LOG

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

   typedef void_type             get_methods_args;
   typedef vector< string >      get_methods_return;

   struct get_signature_args
   {
      string method;
   };

   typedef api_method_signature  get_signature_return;

   class json_rpc_logger
   {
   public:
      json_rpc_logger(const string& _dir_name) : dir_name(_dir_name) {}

      ~json_rpc_logger()
      {
         if (counter == 0)
            return; // nothing to flush

         // flush tests.yaml file with all passed responses
         fc::path file(dir_name);
         file /= "tests.yaml";

         const char* head =
         "---\n"
         "- config:\n"
         "  - testset: \"API Tests\"\n"
         "  - generators:\n"
         "    - test_id: {type: 'number_sequence', start: 1}\n"
         "\n"
         "- base_test: &base_test\n"
         "  - generator_binds:\n"
         "    - test_id: test_id\n"
         "  - url: \"/rpc\"\n"
         "  - method: \"POST\"\n"
         "  - validators:\n"
         "    - extract_test: {jsonpath_mini: \"error\", test: \"not_exists\"}\n"
         "    - extract_test: {jsonpath_mini: \"result\", test: \"exists\"}\n";

         fc::ofstream o(file);
         o << head;
         o << "    - json_file_validator: {jsonpath_mini: \"result\", comparator: \"json_compare\", expected: {template: '" << dir_name << "/$test_id'}}\n\n";

         for (uint32_t i = 1; i <= counter; ++i)
         {
            o << "- test:\n";
            o << "  - body: {file: \"" << i << ".json\"}\n";
            o << "  - name: \"test" << i << "\"\n";
            o << "  - <<: *base_test\n";
            o << "\n";
         }

         o.close();
      }

      void log(const fc::variant_object& request, json_rpc_response& response)
      {
         fc::path file(dir_name);
         bool error = response.error.valid();
         std::string counter_str;

         if (error)
            counter_str = std::to_string(++errors) + "_error";
         else
            counter_str = std::to_string(++counter);

         file /= counter_str + ".json";

         fc::json::save_to_file(request, file);

         file.replace_extension("json.pat");

         if (error)
            fc::json::save_to_file(response.error, file);
         else
            fc::json::save_to_file(response.result, file);
      }

   private:
      string   dir_name;
      /** they are used as 1-based, because of problem with start pyresttest number_sequence generator from 0
       *  (it means first test is '1' also as first error)
       */
      uint32_t counter = 0;
      uint32_t errors = 0;
   };

   class json_rpc_plugin_impl
   {
      public:
         json_rpc_plugin_impl();
         ~json_rpc_plugin_impl();

         void add_api_method( const string& api_name, const string& method_name, const api_method& api, const api_method_signature& sig );

         api_method* find_api_method( std::string api, std::string method );
         api_method* process_params( string method, const fc::variant_object& request, fc::variant& func_args, string* method_name );
         void rpc_id( const fc::variant_object& request, json_rpc_response& response );
         void rpc_jsonrpc( const fc::variant_object& request, json_rpc_response& response );
         json_rpc_response rpc( const fc::variant& message );

         void initialize();

         void log(const fc::variant_object& request, json_rpc_response& response)
         {
            if (_logger)
               _logger->log(request, response);
         }

         DECLARE_API(
            (get_methods)
            (get_signature) )

         map< string, api_description >                     _registered_apis;
         vector< string >                                   _methods;
         map< string, map< string, api_method_signature > > _method_sigs;
         std::unique_ptr< json_rpc_logger >                 _logger;
   };

   json_rpc_plugin_impl::json_rpc_plugin_impl() {}
   json_rpc_plugin_impl::~json_rpc_plugin_impl() {}

   void json_rpc_plugin_impl::add_api_method( const string& api_name, const string& method_name, const api_method& api, const api_method_signature& sig )
   {
      _registered_apis[ api_name ][ method_name ] = api;
      _method_sigs[ api_name ][ method_name ] = sig;

      std::stringstream canonical_name;
      canonical_name << api_name << '.' << method_name;
      _methods.push_back( canonical_name.str() );
   }

   void json_rpc_plugin_impl::initialize()
   {
      JSON_RPC_REGISTER_API( "jsonrpc" );
   }

   get_methods_return json_rpc_plugin_impl::get_methods( const get_methods_args& args, bool lock )
   {
      FC_UNUSED( lock )
      return _methods;
   }

   get_signature_return json_rpc_plugin_impl::get_signature( const get_signature_args& args, bool lock )
   {
      FC_UNUSED( lock )
      vector< string > v;
      boost::split( v, args.method, boost::is_any_of( "." ) );
      FC_ASSERT( v.size() == 2, "Invalid method name" );

      auto api_itr = _method_sigs.find( v[0] );
      FC_ASSERT( api_itr != _method_sigs.end(), "Method ${api}.${method} does not exist.", ("api", v[0])("method", v[1]) );

      auto method_itr = api_itr->second.find( v[1] );
      FC_ASSERT( method_itr != api_itr->second.end(), "Method ${api}.${method} does not exist", ("api", v[0])("method", v[1]) );

      return method_itr->second;
   }

   api_method* json_rpc_plugin_impl::find_api_method( std::string api, std::string method )
   {
      STATSD_START_TIMER( "jsonrpc", "overhead", "find_api_method", 1.0f );
      auto api_itr = _registered_apis.find( api );
      FC_ASSERT( api_itr != _registered_apis.end(), "Could not find API ${api}", ("api", api) );

      auto method_itr = api_itr->second.find( method );
      FC_ASSERT( method_itr != api_itr->second.end(), "Could not find method ${method}", ("method", method) );

      return &(method_itr->second);
   }

   api_method* json_rpc_plugin_impl::process_params( string method, const fc::variant_object& request, fc::variant& func_args, string* method_name )
   {
      STATSD_START_TIMER( "jsonrpc", "overhead", "process_params", 1.0f );
      api_method* ret = nullptr;

      if( method == "call" )
      {
         FC_ASSERT( request.contains( "params" ) );

         std::vector< fc::variant > v;

         if( request[ "params" ].is_array() )
            v = request[ "params" ].as< std::vector< fc::variant > >();

         FC_ASSERT( v.size() == 2 || v.size() == 3, "params should be {\"api\", \"method\", \"args\"" );

         auto api = v[0].as_string();
         auto method = v[1].as_string();

         ret = find_api_method( api, method );

         *method_name = api + "." + method;

         func_args = ( v.size() == 3 ) ? v[2] : fc::json::from_string( "{}" );
      }
      else
      {
         vector< std::string > v;
         boost::split( v, method, boost::is_any_of( "." ) );

         FC_ASSERT( v.size() == 2, "method specification invalid. Should be api.method" );

         ret = find_api_method( v[0], v[1] );

         *method_name = method;

         func_args = request.contains( "params" ) ? request[ "params" ] : fc::json::from_string( "{}" );
      }

      return ret;
   }

   void json_rpc_plugin_impl::rpc_id( const fc::variant_object& request, json_rpc_response& response )
   {
      STATSD_START_TIMER( "jsonrpc", "overhead", "rpc_id", 1.0f );
      if( request.contains( "id" ) )
      {
         const fc::variant& _id = request[ "id" ];
         int _type = _id.get_type();
         switch( _type )
         {
            case fc::variant::int64_type:
            case fc::variant::uint64_type:
            case fc::variant::string_type:
               response.id = request[ "id" ];
            break;

            default:
               response.error = json_rpc_error( JSON_RPC_INVALID_REQUEST, "Only integer value or string is allowed for member \"id\"" );
         }
      }
   }

   void json_rpc_plugin_impl::rpc_jsonrpc( const fc::variant_object& request, json_rpc_response& response )
   {
      STATSD_START_TIMER( "jsonrpc", "overhead", "rpc_jsonrpc", 1.0f );
      if( request.contains( "jsonrpc" ) && request[ "jsonrpc" ].is_string() && request[ "jsonrpc" ].as_string() == "2.0" )
      {
         if( request.contains( "method" ) && request[ "method" ].is_string() )
         {
            try
            {
               string method = request[ "method" ].as_string();

               // This is to maintain backwards compatibility with existing call structure.
               if( ( method == "call" && request.contains( "params" ) ) || method != "call" )
               {
                  fc::variant func_args;
                  api_method* call = nullptr;
                  string method_name;

                  try
                  {
                     call = process_params( method, request, func_args, &method_name );
                  }
                  catch( fc::assert_exception& e )
                  {
                     response.error = json_rpc_error( JSON_RPC_PARSE_PARAMS_ERROR, e.to_string(), fc::variant( *(e.dynamic_copy_exception()) ) );
                  }

                  try
                  {
                     if( call )
                     {
                        STATSD_START_TIMER( "jsonrpc", "api", method_name, 1.0f );
                        response.result = (*call)( func_args );
                     }
                  }
                  catch( chainbase::lock_exception& e )
                  {
                     response.error = json_rpc_error( JSON_RPC_ERROR_DURING_CALL, e.what() );
                  }
                  catch( fc::assert_exception& e )
                  {
                     response.error = json_rpc_error( JSON_RPC_ERROR_DURING_CALL, e.to_string(), fc::variant( *(e.dynamic_copy_exception()) ) );
                  }
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

   log(request, response);
   }

   json_rpc_response json_rpc_plugin_impl::rpc( const fc::variant& message )
   {
      json_rpc_response response;

      ddump( (message) );

      STATSD_START_TIMER( "jsonrpc", "overhead", "total", 1.0f );

      try
      {
         const auto& request = message.get_object();

         rpc_id( request, response );

         // This second layer try/catch is to isolate errors that occur after parsing the id so that the id is properly returned.
         try
         {
            if( !response.error.valid() )
               rpc_jsonrpc( request, response );
         }
         catch( fc::exception& e )
         {
            response.error = json_rpc_error( JSON_RPC_SERVER_ERROR, e.to_string(), fc::variant( *(e.dynamic_copy_exception()) ) );
         }
         catch( std::exception& e )
         {
            response.error = json_rpc_error( JSON_RPC_SERVER_ERROR, "Unknown error - parsing rpc message failed", fc::variant( e.what() ) );
         }
         catch( ... )
         {
            response.error = json_rpc_error( JSON_RPC_SERVER_ERROR, "Unknown error - parsing rpc message failed" );
         }
      }
      catch( fc::parse_error_exception& e )
      {
         response.error = json_rpc_error( JSON_RPC_PARSE_ERROR, e.to_string(), fc::variant( *(e.dynamic_copy_exception()) ) );
      }
      catch( fc::bad_cast_exception& e )
      {
         response.error = json_rpc_error( JSON_RPC_PARSE_ERROR, e.to_string(), fc::variant( *(e.dynamic_copy_exception()) ) );
      }
      catch( fc::exception& e )
      {
         response.error = json_rpc_error( JSON_RPC_SERVER_ERROR, e.to_string(), fc::variant( *(e.dynamic_copy_exception()) ) );
      }
      catch( std::exception& e )
      {
         response.error = json_rpc_error( JSON_RPC_SERVER_ERROR, "Unknown error - parsing rpc message failed", fc::variant( e.what() ) );
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
using detail::json_rpc_logger;

json_rpc_plugin::json_rpc_plugin() : my( new detail::json_rpc_plugin_impl() ) {}
json_rpc_plugin::~json_rpc_plugin() {}

void json_rpc_plugin::set_program_options( options_description& , options_description& cfg)
{
   cfg.add_options()
      ("log-json-rpc", bpo::value< string >(), "json-rpc log directory name.")
      ;
}

void json_rpc_plugin::plugin_initialize( const variables_map& options )
{
   my->initialize();

   if( options.count( "log-json-rpc" ) )
   {
      auto dir_name = options.at( "log-json-rpc" ).as< string >();
      FC_ASSERT(dir_name.empty() == false, "Invalid directory name (empty).");

      fc::path p(dir_name);
      if (fc::exists(p))
         fc::remove_all(p);
      fc::create_directories(p);
      my->_logger.reset(new json_rpc_logger(dir_name));
   }
}

void json_rpc_plugin::plugin_startup()
{
   std::sort( my->_methods.begin(), my->_methods.end() );
}

void json_rpc_plugin::plugin_shutdown() {}

void json_rpc_plugin::add_api_method( const string& api_name, const string& method_name, const api_method& api, const api_method_signature& sig )
{
   my->add_api_method( api_name, method_name, api, sig );
}

string json_rpc_plugin::call( const string& message )
{
   STATSD_START_TIMER( "jsonrpc", "overhead", "call", 1.0f );
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
   catch( ... )
   {
      json_rpc_response response;
      response.error = json_rpc_error( JSON_RPC_SERVER_ERROR, "Unknown exception", fc::variant(
         fc::unhandled_exception( FC_LOG_MESSAGE( warn, "Unknown Exception" ), std::current_exception() ).to_detail_string() ) );
      return fc::json::to_string( response );
   }

}

} } } // steem::plugins::json_rpc

FC_REFLECT( steem::plugins::json_rpc::detail::json_rpc_error, (code)(message)(data) )
FC_REFLECT( steem::plugins::json_rpc::detail::json_rpc_response, (jsonrpc)(result)(error)(id) )

FC_REFLECT( steem::plugins::json_rpc::detail::get_signature_args, (method) )

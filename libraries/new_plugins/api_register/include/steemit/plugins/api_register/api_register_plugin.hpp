#pragma once

#include <appbase/application.hpp>

#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/exception/exception.hpp>

#include <boost/config.hpp>
#include <boost/any.hpp>

#define API_REGISTER_PLUGIN_NAME "api_register"

#define API_MEMBER( handle )                                \
{ std::string( #handle ),                                   \
   [this]( fc::variant args ) -> fc::variant                \
   {                                                        \
      return this->handle( args.as< handle ## _args >() );  \
   }                                                        \
}

namespace steemit { namespace plugins { namespace api_register {

using namespace appbase;

struct api_void_args {};

namespace detail
{
   /**
    * @brief Callback type for a URL handler
    *
    * URL handlers have this type
    *
    * The handler must gaurantee that url_response_callback() is called;
    * otherwise, the connection will hang and result in a memory leak.
    *
    * Arguments: url, request_body, response_callback
    */
   using api_call = std::function< fc::variant(fc::variant) >;

   /**
    * @brief An API, containing URLs and handlers
    *
    * An API is composed of several calls, where each call has a URL and
    * a handler. The URL is the path on the web server that triggers the
    * call, and the handler is the function which implements the API call
    */
   using api_description = std::map<string, api_call>;

   struct json_rpc_request
   {
      std::string                      jsonrpc;
      std::string                      method;
      std::string                      params;
      fc::optional< std::string >      id;
   };

   struct json_rpc_error
   {
      json_rpc_error( int32_t c, std::string m, fc::optional< std::string > d = fc::optional< std::string >() )
         : code( c ), message( m ), data( d ) {}

      int32_t                          code;
      std::string                      message;
      fc::optional< std::string >      data;
   };

   struct json_rpc_response
   {
      std::string                      jsonrpc = "2.0";
      fc::optional< std::string >      result;
      fc::optional< json_rpc_error >   error;
      fc::variant                      id;
   };
}

class api_register_plugin : public appbase::plugin< api_register_plugin >
{
   public:
      api_register_plugin();
      virtual ~api_register_plugin();

      APPBASE_PLUGIN_REQUIRES();
      virtual void set_program_options( options_description&, options_description& ) override {}

      void plugin_initialize( const variables_map& options );
      void plugin_startup();
      void plugin_shutdown();

      void add_api( const string& api_name, const detail::api_description& api )
      {
         _registered_apis[ api_name ] = api;
      }

      string call_api( string& body );

   private:
      map< string, detail::api_description > _registered_apis;
};

} } } // steemit::plugins::api_register

FC_REFLECT( steemit::plugins::api_register::detail::json_rpc_request, (jsonrpc)(method)(params)(id) )
FC_REFLECT( steemit::plugins::api_register::detail::json_rpc_error, (code)(message)(data) )
FC_REFLECT( steemit::plugins::api_register::detail::json_rpc_response, (jsonrpc)(result)(error)(id) )

FC_REFLECT( steemit::plugins::api_register::api_void_args, )

/*
#define PLUGIN_INITIALIZE_API( handles... )                                                     \
   appbase::app().get_plugin< steemit::plugins::api_register::api_register_plugin >().add_api(  \
      this,                                                                                     \
      {                                                                                         \
         BOOST_PP_SEQ_FOR_EACH( PLUGIN_INITIALIZE_API_MEMBER, _, handles ),                                                   \
      }
*/

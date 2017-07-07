#pragma once

#include <appbase/application.hpp>

#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/exception/exception.hpp>

#include <boost/config.hpp>
#include <boost/any.hpp>

/**
 * This plugin holds bindings for all APIs and their methods
 * and can dispatch JSONRPC requests to the appropriate API.
 *
 * For a plugin to use the API Register, it needs to specify
 * the register as a dependency. Then, during initializtion,
 * register itself using add_api.
 *
 * Ex.
 * appbase::app().get_plugin< api_register_plugin >().add_api(
 *    name(),
 *    {
 *       API_METHOD( method_1 ),
 *       API_METHOD( method_2 ),
 *       API_METHOD( method_3 )
 *    });
 *
 * All method should take a single struct as an argument called
 * method_1_args, method_2_args, method_3_args, etc. and should
 * return a single struct as a return type.
 *
 * For methods that do not require arguments, use api_void_args
 * as the argument type.
 */

#define API_REGISTER_PLUGIN_NAME "api_register"

#define API_METHOD( handle )                                                        \
{ std::string( #handle ),                                                           \
   [this]( const fc::variant& args ) -> std::string                                 \
   {                                                                                \
      return fc::json::to_string( this->handle( args.as< handle ## _args >() ) );   \
   }                                                                                \
}

namespace steemit { namespace plugins { namespace api_register {

using namespace appbase;

/**
 * Void argument type for API calls
 */
struct api_void_args {};

namespace detail
{
   /**
    * @brief Internal type used to bind api methods
    * to names.
    *
    * Arguments: Variant object of propert arg type
    */
   using api_call = std::function< std::string(const fc::variant&) >;

   /**
    * @brief An API, containing APIs and Methods
    *
    * An API is composed of several calls, where each call has a
    * name defined by the API class. The api_call functions
    * are compile time bindings of names to methods.
    */
   using api_description = std::map<string, api_call>;

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

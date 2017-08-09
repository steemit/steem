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
 * appbase::app().get_plugin< json_rpc_plugin >().add_api(
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

#define STEEM_JSON_RPC_PLUGIN_NAME "json_rpc"

#define JSON_RPC_API_METHOD_HELPER( handle )                               \
   [this]( const fc::variant& args ) -> fc::variant                        \
   {                                                                       \
      return fc::variant( this->handle( args.as< handle ## _args >() ) );  \
   }

#define JSON_RPC_API_METHOD( r, api_name, method ) \
   jsonrpc.add_api_method( api_name, std::string( #method ), JSON_RPC_API_METHOD_HELPER( method ) );


#define JSON_RPC_REGISTER_API( API_NAME, METHODS )                                                       \
{                                                                                               \
   auto& jsonrpc = appbase::app().get_plugin< steemit::plugins::json_rpc::json_rpc_plugin >();  \
   BOOST_PP_SEQ_FOR_EACH( JSON_RPC_API_METHOD, API_NAME, METHODS )                              \
}

namespace steemit { namespace plugins { namespace json_rpc {

using namespace appbase;

/**
 * @brief Internal type used to bind api methods
 * to names.
 *
 * Arguments: Variant object of propert arg type
 */
typedef std::function< fc::variant(const fc::variant&) > api_method;

/**
 * @brief An API, containing APIs and Methods
 *
 * An API is composed of several calls, where each call has a
 * name defined by the API class. The api_call functions
 * are compile time bindings of names to methods.
 */
typedef std::map< string, api_method > api_description;

namespace detail
{
   class json_rpc_plugin_impl;



   /*struct json_rpc_error
   {
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
   };*/
}

class json_rpc_plugin : public appbase::plugin< json_rpc_plugin >
{
   public:
      json_rpc_plugin();
      virtual ~json_rpc_plugin();

      APPBASE_PLUGIN_REQUIRES();
      virtual void set_program_options( options_description&, options_description& ) override {}

      void plugin_initialize( const variables_map& options );
      void plugin_startup();
      void plugin_shutdown();

      void add_api_method( const string& api_name, const string& method_name, const api_method& api );
      string call( const string& body );

   private:
      std::shared_ptr< detail::json_rpc_plugin_impl >   _my;
};

} } } // steemit::plugins::json_rpc

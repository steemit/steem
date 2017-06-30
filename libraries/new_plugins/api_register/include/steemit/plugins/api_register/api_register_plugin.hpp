#pragma once

#include <appbase/application.hpp>

#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/exception/exception.hpp>

#include <boost/config.hpp>
#include <boost/any.hpp>

#define API_REGISTER_PLUGIN_NAME "api_register"

namespace steemit { namespace plugins { namespace api_register {

using namespace appbase;

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
   using api_call = std::function< fc::variant(string) >;

   /**
    * @brief An API, containing URLs and handlers
    *
    * An API is composed of several calls, where each call has a URL and
    * a handler. The URL is the path on the web server that triggers the
    * call, and the handler is the function which implements the API call
    */
   using api_description = std::map<string, api_call>;
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

#define API_MEMBER( handle )                                                        \
{ std::string( #handle ),                                      \
   [this]( string body ) -> fc::variant                                             \
   { \
      try \
      {                                                            \
      if( body.empty() ) body = "{}";                                               \
      return this->handle( fc::json::from_string( body ).as< handle ## _args >() ); \
      } \
      FC_LOG_AND_RETHROW() \
   }                                                                                \
}

/*
#define PLUGIN_INITIALIZE_API( handles... )                                                     \
   appbase::app().get_plugin< steemit::plugins::api_register::api_register_plugin >().add_api(  \
      this,                                                                                     \
      {                                                                                         \
         BOOST_PP_SEQ_FOR_EACH( PLUGIN_INITIALIZE_API_MEMBER, _, handles ),                                                   \
      }
*/

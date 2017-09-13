#pragma once
#include <appbase/application.hpp>

#include <steem/plugins/json_rpc/json_rpc_plugin.hpp>

#define STEEM_EXAMPLE_API_PLUGIN_NAME "example_api"

namespace steem { namespace example_api_plugin {

   using namespace appbase;


   // Define API method arg and return types
   typedef json_rpc::void_type hello_world_args;

   struct hello_world_return
   {
      string message;
   };


   struct echo_args
   {
      string call;
   };

   struct echo_return
   {
      string response;
   }


   // All plugins must inherit from appbase::plugin
   class example_api_plugin : public appbase::plugin< example_api_plugin >
   {
      public:
         example_api_plugin();
         virtual ~example_api_plugin();

         // This defines what plugins are required to run this plugin.
         // These plugins will load before this one and shutdown after.
         APPBASE_PLUGIN_REQUIRES( (plugins::json_rpc::json_rpc_plugin) );

         // This static method is a required by the appbase::plugin template
         static const std::string& name() { static std::string name = STEEM_EXAMPLE_API_PLUGIN_NAME; return name; }

         // Specify any config options here
         virtual void set_program_options( options_description&, options_description& ) override {}

         // These implement startup and shutdown logic for the plugin.
         // plugin_initialize and plugin_startup are called such that dependencies go first
         // plugin_shutdown goes in reverse order such the dependencies are running when shutting down.
         virtual void plugin_initialize( const variables_map& options ) override;
         virtual void plugin_startup() override;
         virtual void plugin_shutdown() override;

         // These are the API methods defined for the plugin
         // APIs take struct args and return structs
         hello_world_return hello_world( const hello_world_args& args );
         echo_return echo( const echo_args& args );
   };

   example_api_plugin::example_api_plugin() {}
   example_api_plugin::~example_api_plugin() {}

   void example_api_plugin::plugin_initialize( const variables_map& options )
   {
      // This registers the API with the json rpc plugin
      JSON_RPC_REGISTER_API( name(), (hello_world)(echo) );
   }

   void example_api_plugin::plugin_startup() {}
   void example_api_plugin::plugin_shutdown() {}

   hello_world_return hello_world( const hello_world_args& args )
   {
      return hello_world_return{ "Hello World" };
   }

   echo_return echo( const echo_args& args )
   {
      return echo_return{ args.call };
   }

} } // steem::example_api_plugin

// Args and return types need to be reflected. hello_world_args does not because it is a typedef of a reflected type
FC_REFLECT( steem::example_api_plugin::hello_world_return, (message) )
FC_REFLECT( steem::example_api_plugin::echo_args, (call) )
FC_REFLECT( steem::example_api_plugin::echo_return, (response) )

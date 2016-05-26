
#include <steemit/app/application.hpp>
#include <steemit/app/plugin.hpp>

#include <sstream>
#include <string>

namespace steemit { namespace example_plugin {

class hello_api_plugin : public steemit::app::plugin
{
   public:
      /**
       * The plugin requires a nullary constructor.  This is called regardless of whether the plugin is loaded.
       * Base class functionality such as app() is not fully initialized when the constructor is called.
       * Most plugins should have an empty constructor and do initialization logic in plugin_initialize()
       * or plugin_startup() instead.
       */
      hello_api_plugin();

      /**
       * Plugin is destroyed via base class pointer, so a virtual destructor must be provided.
       */
      virtual ~hello_api_plugin();

      /**
       * Every plugin needs a name.
       */
      virtual std::string plugin_name()const override;

      /**
       * Called when the plugin is enabled, but before the database has been created.
       */
      virtual void plugin_initialize( const boost::program_options::variables_map& options ) override;

      /**
       * Called when the plugin is enabled.
       */
      virtual void plugin_startup() override;

      std::string get_message();

   private:
      std::string _message;
      uint32_t _plugin_call_count = 0;
};

class hello_api_api
{
   public:
      hello_api_api( const steemit::app::api_context& ctx );

      /**
       * Called immediately after the constructor.  If the API class uses enable_shared_from_this,
       * shared_from_this() is available in this method, which allows signal handlers to be registered
       * with app::connect_signal()
       */
      void on_api_startup();
      std::string get_message();

   private:
      steemit::app::application& _app;
      uint32_t _api_call_count = 0;
};

} }

FC_API( steemit::example_plugin::hello_api_api,
   (get_message)
   )

namespace steemit { namespace example_plugin {

hello_api_plugin::hello_api_plugin() {}
hello_api_plugin::~hello_api_plugin() {}

std::string hello_api_plugin::plugin_name()const
{
   return "hello_api";
}

void hello_api_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   _message = "hello world";
}

void hello_api_plugin::plugin_startup()
{
   app().register_api_factory< hello_api_api >( "hello_api_api" );
}

std::string hello_api_plugin::get_message()
{
   std::stringstream result;
   result << _message << " -- users have viewed this message " << (_plugin_call_count++) << " times";
   return result.str();
}

hello_api_api::hello_api_api( const steemit::app::api_context& ctx ) : _app(ctx.app) {}

void hello_api_api::on_api_startup() {}

std::string hello_api_api::get_message()
{
   std::stringstream result;
   std::shared_ptr< hello_api_plugin > plug = _app.get_plugin< hello_api_plugin >( "hello_api" );
   result << plug->get_message() << " -- you've viewed this message " << (_api_call_count++) << " times";
   return result.str();
}

} }

/**
 * The STEEMIT_DEFINE_PLUGIN() macro will define a steemit::plugin::create_hello_api_plugin()
 * factory method which is expected by the manifest.
 */

STEEMIT_DEFINE_PLUGIN( hello_api, steemit::example_plugin::hello_api_plugin )

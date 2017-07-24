#pragma once
#include <appbase/application.hpp>

#include <steemit/plugins/json_rpc/json_rpc_plugin.hpp>

#include <boost/thread.hpp>
#include <boost/container/vector.hpp>

#define WEBSERVER_PLUGIN_NAME "webserver"

namespace steemit { namespace plugins { namespace webserver {

using namespace appbase;

/**
  * This plugin starts an HTTP/ws webserver and dispatches queries to
  * registered handles based on payload. The payload must be conform
  * to the JSONRPC 2.0 spec.
  *
  * The handler will be called from the appbase application io_service
  * thread.  The callback can be called from any thread and will
  * automatically propagate the call to the http thread.
  *
  * The HTTP service will run in its own thread with its own io_service to
  * make sure that HTTP request processing does not interfer with other
  * plugins.
  */
class webserver_plugin : public appbase::plugin< webserver_plugin >
{
   public:
      webserver_plugin();
      virtual ~webserver_plugin();

      APPBASE_PLUGIN_REQUIRES( (plugins::json_rpc::json_rpc_plugin) );
      virtual void set_program_options(options_description&, options_description& cfg) override;

      void plugin_initialize(const variables_map& options);
      void plugin_startup();
      void plugin_shutdown();

   private:
      std::unique_ptr< class webserver_plugin_impl > _my;
};

} } } // steemit::plugins::webserver

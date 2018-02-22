#pragma once

#include <appbase/application.hpp>

#include <golos/plugins/json_rpc/plugin.hpp>

#include <boost/thread.hpp>
#include <boost/container/vector.hpp>


#define STEEM_WEBSERVER_PLUGIN_NAME "webserver"

namespace golos {
    namespace plugins {
        namespace webserver {

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
            class webserver_plugin final : public appbase::plugin<webserver_plugin> {
            public:
                webserver_plugin();

                ~webserver_plugin();

                APPBASE_PLUGIN_REQUIRES((json_rpc::plugin));

                static const std::string &name() {
                    static std::string name = STEEM_WEBSERVER_PLUGIN_NAME;
                    return name;
                }

                void set_program_options(boost::program_options::options_description &, boost::program_options::options_description &cfg) override;

            protected:
                void plugin_initialize(const boost::program_options::variables_map &options) override;

                void plugin_startup() override;

                void plugin_shutdown() override;

            private:
                struct webserver_plugin_impl;
                std::unique_ptr<webserver_plugin_impl> my;
            };

        }
    }
} // steem::plugins::webserver

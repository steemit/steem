#pragma once

#include <golos/plugins/json_rpc/plugin.hpp>
#include <memory>

namespace golos {
    namespace plugins {
        namespace tolstoy_api {

            using namespace appbase;

            class tolstoy_api_plugin final : public appbase::plugin<tolstoy_api_plugin> {
            public:
                APPBASE_PLUGIN_REQUIRES((json_rpc::plugin))

                tolstoy_api_plugin();

                ~tolstoy_api_plugin();

                constexpr static const char *plugin_name = "tolstoy_api";

                static const std::string &name() {
                    static std::string name = plugin_name;
                    return name;
                }

                void set_program_options(boost::program_options::options_description &cli, boost::program_options::options_description &cfg) override;

                void plugin_initialize(const boost::program_options::variables_map &options) override;

                void plugin_startup() override;

                void plugin_shutdown() override;

            private:
                std::unique_ptr<class tolstoy_api> api;
            };

        }
    }
} // steem::plugins::condenser_api

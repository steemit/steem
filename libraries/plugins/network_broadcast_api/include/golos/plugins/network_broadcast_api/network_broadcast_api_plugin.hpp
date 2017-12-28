#pragma once

#include <golos/plugins/json_rpc/plugin.hpp>
#include <golos/plugins/chain/plugin.hpp>
#include <golos/plugins/p2p/p2p_plugin.hpp>

#include <appbase/application.hpp>

#define STEEM_NETWORK_BROADCAST_API_PLUGIN_NAME "network_broadcast_api"

namespace golos {
    namespace plugins {
        namespace network_broadcast_api {

            using namespace appbase;

            class network_broadcast_api_plugin final : public appbase::plugin<network_broadcast_api_plugin> {
            public:
                APPBASE_PLUGIN_REQUIRES((json_rpc::plugin) (chain::plugin) (p2p::p2p_plugin))

                network_broadcast_api_plugin();

                ~network_broadcast_api_plugin();

                static const std::string &name() {
                    static std::string name = STEEM_NETWORK_BROADCAST_API_PLUGIN_NAME;
                    return name;
                }

                void set_program_options(boost::program_options::options_description &cli, boost::program_options::options_description &cfg) override;

                void plugin_initialize(const boost::program_options::variables_map &options) override;

                void plugin_startup() override;

                void plugin_shutdown() override;

                boost::signals2::connection on_applied_block_connection;

                std::shared_ptr<class network_broadcast_api> api;
            };

        }
    }
}

#pragma once


#include <golos/chain/database.hpp>
#include <boost/program_options/variables_map.hpp>
#include <appbase/application.hpp>
#include <golos/plugins/chain/plugin.hpp>
#include <golos/plugins/p2p/p2p_plugin.hpp>
#include <memory>

namespace golos {
    namespace plugins {
        namespace witness_plugin {

            using std::string;
            using protocol::public_key_type;
            using golos::protocol::block_id_type;
            using golos::chain::signed_block;

            namespace block_production_condition {
                enum block_production_condition_enum {
                    produced = 0,
                    not_synced = 1,
                    not_my_turn = 2,
                    not_time_yet = 3,
                    no_private_key = 4,
                    low_participation = 5,
                    lag = 6,
                    consecutive = 7,
                    wait_for_genesis = 8,
                    exception_producing_block = 9
                };
            }

            class witness_plugin final : public appbase::plugin<witness_plugin> {
            public:
                APPBASE_PLUGIN_REQUIRES((chain::plugin) (p2p::p2p_plugin))

                constexpr static const char *plugin_name = "witness";

                static const std::string &name() {
                    static std::string name = plugin_name;
                    return name;
                }

                witness_plugin();

                ~witness_plugin();


                void set_program_options(boost::program_options::options_description &command_line_options,
                                         boost::program_options::options_description &config_file_options) override;

                void set_block_production(bool allow);

                void plugin_initialize(const boost::program_options::variables_map &options) override;

                void plugin_startup() override;

                void plugin_shutdown() override;

            private:
                struct impl;
                std::unique_ptr<impl> pimpl;

            };

        }
    }
} //golos::witness_plugin

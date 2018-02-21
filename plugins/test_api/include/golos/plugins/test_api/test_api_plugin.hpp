#pragma once

#include <appbase/application.hpp>

#include <golos/plugins/json_rpc/plugin.hpp>
#include <golos/plugins/json_rpc/utility.hpp>

namespace golos {
    namespace plugins {
        namespace test_api {

            using namespace appbase;
            using json_rpc::msg_pack;
            using fc::variant;

            struct test_api_a_t {
                std::string value;
            };
            struct test_api_b_t {
                std::string value;
            };

            ///               API,        args,       return
            DEFINE_API_ARGS(test_api_a, msg_pack, test_api_a_t)
            DEFINE_API_ARGS(test_api_b, msg_pack, test_api_b_t)

            class test_api_plugin final : public appbase::plugin<test_api_plugin> {
            public:
                test_api_plugin();

                ~test_api_plugin();

                constexpr static const char *plugin_name = "test_api";

                APPBASE_PLUGIN_REQUIRES((json_rpc::plugin));

                static const std::string &name() {
                    static std::string name = plugin_name;
                    return name;
                }

                void set_program_options(boost::program_options::options_description &,
                                         boost::program_options::options_description &) override {
                }

                void plugin_initialize(const boost::program_options::variables_map &options) override;

                void plugin_startup() override;

                void plugin_shutdown() override;

                DECLARE_API((test_api_a)(test_api_b))
            };

        }
    }
} // steem::plugins::test_api

FC_REFLECT((golos::plugins::test_api::test_api_a_t), (value))
FC_REFLECT((golos::plugins::test_api::test_api_b_t), (value))

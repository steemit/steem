#include <golos/plugins/test_api/test_api_plugin.hpp>

#include <fc/log/logger_config.hpp>

namespace golos {
    namespace plugins {
        namespace test_api {

            test_api_plugin::test_api_plugin() {
            }

            test_api_plugin::~test_api_plugin() {
            }

            void test_api_plugin::plugin_initialize(const boost::program_options::variables_map &options) {
                JSON_RPC_REGISTER_API(plugin_name);
            }

            void test_api_plugin::plugin_startup() {
            }

            void test_api_plugin::plugin_shutdown() {
            }

            DEFINE_API(test_api_plugin, test_api_a) {
                test_api_a_t result;
                result.value = "A";
                return result;
            }

            DEFINE_API(test_api_plugin, test_api_b) {
                test_api_b_t result;
                result.value = "B";
                return result;
            }

        }
    }
} // steem::plugins::test_api

#pragma once

#include <appbase/application.hpp>
#include <golos/plugins/json_rpc/utility.hpp>
#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/exception/exception.hpp>

#include <boost/config.hpp>
#include <boost/any.hpp>

/**
 * This plugin holds bindings for all APIs and their methods
 * and can dispatch JSONRPC requests to the appropriate API.
 *
 * For a plugin to use the API Register, it needs to specify
 * the register as a dependency. Then, during initializtion,
 * register itself using add_api.
 *
 * Ex.
 * appbase::app().get_plugin< json_rpc_plugin >().add_api(
 *    name(),
 *    {
 *       API_METHOD( method_1 ),
 *       API_METHOD( method_2 ),
 *       API_METHOD( method_3 )
 *    });
 *
 * All method should take a single struct as an argument called
 * method_1_args, method_2_args, method_3_args, etc. and should
 * return a single struct as a return type.
 *
 * For methods that do not require arguments, use api_void_args
 * as the argument type.
 */

#define STEEM_JSON_RPC_PLUGIN_NAME "json_rpc"

#define JSON_RPC_REGISTER_API(API_NAME)                                                       \
{                                                                                               \
   golos::plugins::json_rpc::detail::register_api_method_visitor vtor( API_NAME );            \
   for_each_api( vtor );                                                                        \
}

#define JSON_RPC_PARSE_ERROR        (-32700)
#define JSON_RPC_INVALID_REQUEST    (-32600)
#define JSON_RPC_METHOD_NOT_FOUND   (-32601)
#define JSON_RPC_INVALID_PARAMS     (-32602)
#define JSON_RPC_INTERNAL_ERROR     (-32603)
#define JSON_RPC_SERVER_ERROR       (-32000)
#define JSON_RPC_NO_PARAMS          (-32001)
#define JSON_RPC_PARSE_PARAMS_ERROR (-32002)
#define JSON_RPC_ERROR_DURING_CALL  (-32003)

namespace golos {
    namespace plugins {
        namespace json_rpc {

            using namespace appbase;

            /**
             * @brief Internal type used to bind api methods
             * to names.
             *
             * Arguments: Variant object of propert arg type
             */
            using api_method = std::function<fc::variant(msg_pack &)>;

            /**
             * @brief An API, containing APIs and Methods
             *
             * An API is composed of several calls, where each call has a
             * name defined by the API class. The api_call functions
             * are compile time bindings of names to methods.
             */
            using api_description = std::map<string, api_method>;

            struct api_method_signature {
                fc::variant args;
                fc::variant ret;
            };

            class plugin final : public appbase::plugin<plugin> {
            public:
                using response_handler_type = std::function<void (const std::string &)>;

                plugin();

                ~plugin();

                APPBASE_PLUGIN_REQUIRES();

                void set_program_options(boost::program_options::options_description &,
                                         boost::program_options::options_description &) override {
                }

                static const std::string &name() {
                    static std::string name = STEEM_JSON_RPC_PLUGIN_NAME;
                    return name;
                }

                void plugin_initialize(const boost::program_options::variables_map &options) override;

                void plugin_startup() override;

                void plugin_shutdown() override;

                void add_api_method(const string &api_name, const string &method_name,
                                    const api_method &api/*, const api_method_signature& sig */);

                void call(const string &body, response_handler_type);

            private:
                class impl;

                std::unique_ptr<impl> pimpl;
            };

            namespace detail {
                class register_api_method_visitor {
                public:
                    register_api_method_visitor(const std::string &api_name) : _api_name(api_name),
                            _json_rpc_plugin(appbase::app().get_plugin< json_rpc::plugin >()) {
                    }

                    template<typename Plugin, typename Method, typename Args, typename Ret>
                    void operator()(Plugin &plugin, const std::string &method_name, Method method, Args *args,
                                    Ret *ret) {
                        _json_rpc_plugin.add_api_method(_api_name, method_name,
                                                        [&plugin, method](msg_pack &args) -> fc::variant {
                                                            return fc::variant((plugin.*method)(args));
                                                        });
                        /*api_method_signature{ fc::variant( Args() ), fc::variant( Ret() ) }*/ //);
                    }

                private:
                    std::string _api_name;
                    json_rpc::plugin &_json_rpc_plugin;
                };
            }
        }
    }
} // steem::plugins::json_rpc

FC_REFLECT((golos::plugins::json_rpc::api_method_signature), (args)(ret))
#include <golos/plugins/json_rpc/plugin.hpp>
#include <golos/plugins/json_rpc/utility.hpp>

#include <boost/algorithm/string.hpp>

#include <fc/log/logger_config.hpp>
#include <fc/exception/exception.hpp>

namespace golos {
    namespace plugins {
        namespace json_rpc {
            struct json_rpc_error {
                json_rpc_error() : code(0) {
                }

                json_rpc_error(int32_t c, std::string m, fc::optional<fc::variant> d = fc::optional<fc::variant>())
                        : code(c), message(m), data(d) {
                }

                int32_t code;
                std::string message;
                fc::optional<fc::variant> data;
            };

            struct json_rpc_response {
                std::string jsonrpc = "2.0";
                fc::optional<fc::variant> result;
                fc::optional<json_rpc_error> error;
                fc::variant id;
            };

            using get_methods_args     = void_type;
            using get_methods_return   = vector<string>;
            using get_signature_args   = string;
            using get_signature_return = api_method_signature;

            class plugin::plugin_impl {
            public:
                plugin_impl() {
                }

                ~plugin_impl() {
                }

                void add_api_method(const string &api_name, const string &method_name,
                                    const api_method &api/*, const api_method_signature& sig*/ ) {
                    _registered_apis[api_name][method_name] = api;
                    // _method_sigs[ api_name ][ method_name ] = sig;
                    add_method_reindex(api_name, method_name);
                    std::stringstream canonical_name;
                    canonical_name << api_name << '.' << method_name;
                    _methods.push_back(canonical_name.str());
                }

                api_method *find_api_method(std::string api, std::string method) {
                    auto api_itr = _registered_apis.find(api);
                    FC_ASSERT(api_itr != _registered_apis.end(), "Could not find API ${api}", ("api", api));

                    auto method_itr = api_itr->second.find(method);
                    FC_ASSERT(method_itr != api_itr->second.end(), "Could not find method ${method}",
                              ("method", method));

                    return &(method_itr->second);
                }

                api_method *process_params(string method, const fc::variant_object &request, msg_pack &func_args) {
                    api_method *ret = nullptr;

                    if (method == "call") {
                        FC_ASSERT(request.contains("params"));

                        std::vector<fc::variant> v;

                        if (request["params"].is_array()) {
                            v = request["params"].as<std::vector<fc::variant> >();
                        }

                        FC_ASSERT(v.size() == 2 || v.size() == 3, "params should be {\"api\", \"method\", \"args\"");

                        ret = find_api_method(v[0].as_string(), v[1].as_string());
                        func_args.plugin = v[0].as_string();
                        func_args.method = v[1].as_string();
                        fc::variant tmp = (v.size() == 3) ? v[2] : fc::json::from_string("{}");
                        func_args.args = tmp.as<std::vector<fc::variant>>();
                    } else {
                        vector<std::string> v;
                        boost::split(v, method, boost::is_any_of("."));

                        FC_ASSERT(v.size() == 2, "method specification invalid. Should be api.method");

                        ret = find_api_method(v[0], v[1]);
                        func_args.plugin = v[0];
                        func_args.method = v[1];
                        fc::variant tmp = (v.size() == 3) ? v[2] : fc::json::from_string("{}");
                        func_args.args = tmp.as<std::vector<fc::variant>>();
                    }

                    return ret;
                }

                void rpc_id(const fc::variant_object &request, json_rpc_response &response) {
                    if (request.contains("id")) {
                        const fc::variant &_id = request["id"];
                        int _type = _id.get_type();
                        switch (_type) {
                            case fc::variant::int64_type:
                            case fc::variant::uint64_type:
                            case fc::variant::string_type:
                                response.id = request["id"];
                                break;

                            default:
                                response.error = json_rpc_error(JSON_RPC_INVALID_REQUEST,
                                                                "Only integer value or string is allowed for member \"id\"");
                        }
                    }
                }

                void rpc_jsonrpc(const fc::variant_object &request, json_rpc_response &response) {
                    if (request.contains("jsonrpc") && request["jsonrpc"].as_string() == "2.0") {
                        if (request.contains("method")) {
                            try {
                                string method = request["method"].as_string();

                                // This is to maintain backwards compatibility with existing call structure.
                                if ((method == "call" && request.contains("params")) || method != "call") {
                                    msg_pack func_args;
                                    api_method *call = nullptr;

                                    try {
                                        call = process_params(method, request, func_args);
                                    } catch (fc::assert_exception &e) {
                                        response.error = json_rpc_error(JSON_RPC_PARSE_PARAMS_ERROR, e.to_string(),
                                                                        fc::variant(*(e.dynamic_copy_exception())));
                                    }

                                    try {
                                        if (call) {
                                            response.result = (*call)(func_args);
                                        }
                                    } catch (fc::assert_exception &e) {
                                        response.error = json_rpc_error(JSON_RPC_ERROR_DURING_CALL, e.to_string(),
                                                                        fc::variant(*(e.dynamic_copy_exception())));
                                    }
                                } else {
                                    response.error = json_rpc_error(JSON_RPC_NO_PARAMS,
                                                                    "A member \"params\" does not exist");
                                }
                            } catch (fc::assert_exception &e) {
                                response.error = json_rpc_error(JSON_RPC_METHOD_NOT_FOUND, e.to_string(),
                                                                fc::variant(*(e.dynamic_copy_exception())));
                            }
                        } else {
                            response.error = json_rpc_error(JSON_RPC_INVALID_REQUEST,
                                                            "A member \"method\" does not exist");
                        }
                    } else {
                        response.error = json_rpc_error(JSON_RPC_INVALID_REQUEST, "jsonrpc value is not \"2.0\"");
                    }
                }

#define ddump(SEQ) \
                    dlog( FC_FORMAT(SEQ), FC_FORMAT_ARG_PARAMS(SEQ) )

                json_rpc_response rpc(const fc::variant &message) {
                    json_rpc_response response;

                    ddump((message));

                    try {
                        const auto &request = message.get_object();

                        rpc_id(request, response);
                        if (!response.error.valid()) {
                            rpc_jsonrpc(request, response);
                        }
                    } catch (fc::parse_error_exception &e) {
                        response.error = json_rpc_error(JSON_RPC_INVALID_PARAMS, e.to_string(),
                                                        fc::variant(*(e.dynamic_copy_exception())));
                    } catch (fc::bad_cast_exception &e) {
                        response.error = json_rpc_error(JSON_RPC_INVALID_PARAMS, e.to_string(),
                                                        fc::variant(*(e.dynamic_copy_exception())));
                    } catch (fc::exception &e) {
                        response.error = json_rpc_error(JSON_RPC_SERVER_ERROR, e.to_string(),
                                                        fc::variant(*(e.dynamic_copy_exception())));
                    } catch (...) {
                        response.error = json_rpc_error(JSON_RPC_SERVER_ERROR,
                                                        "Unknown error - parsing rpc message failed");
                    }

                    return response;
                }


                void initialize() {

                }

                void add_method_reindex (const std::string & plugin_name, const std::string & method_name) {
                    auto method_itr = _method_reindex.find( method_name );

                    if ( method_itr == _method_reindex.end() ) {
                        _method_reindex[method_name] = plugin_name;
                    }
                    // We don't need to do anything. As we've already added par
                }

                std::string get_methods_parent_plugin (const std::string & method_name) {
                    auto method_itr = _method_reindex.find( method_name );

                    FC_ASSERT(method_itr != _method_reindex.end(), "Could not find method ${method_name}", ("method_name", method_name));

                    return _method_reindex[method_name];                        
                }

                map<string, api_description> _registered_apis;
                vector<string> _methods;
                map<string, map<string, api_method_signature> > _method_sigs;
            private:
                // This is a reindex which allows to get parent plugin by method
                // unordered_map[method] -> plugin
                // For example:
                // Let's get a tolstoy_api get_dynamic_global_properties method
                // So, when we trying to call it, we actually have to call database_api get_dynamic_global_properties
                // That's why we need to store method's parent. 
                std::unordered_map < std::string, std::string> _method_reindex;
            };

            plugin::plugin() : my(new plugin_impl()) {
            }

            plugin::~plugin() {
            }

            void plugin::plugin_initialize(const boost::program_options::variables_map &options) {
                my->initialize();
            }

            void plugin::plugin_startup() {
                std::sort(my->_methods.begin(), my->_methods.end());
            }

            void plugin::plugin_shutdown() {
            }

            void plugin::add_api_method(const string &api_name, const string &method_name,
                                        const api_method &api/*, const api_method_signature& sig */) {
                my->add_api_method(api_name, method_name, api/*, sig*/ );
            }

            string plugin::call(const string &message) {
                try {
                    fc::variant v = fc::json::from_string(message);

                    if (v.is_array()) {
                        vector<fc::variant> messages = v.as<vector<fc::variant> >();
                        vector<json_rpc_response> responses;

                        if (messages.size()) {
                            responses.reserve(messages.size());

                            for (auto &m : messages) {
                                responses.push_back(my->rpc(m));
                            }

                            return fc::json::to_string(responses);
                        } else {
                            //For example: message == "[]"
                            json_rpc_response response;
                            response.error = json_rpc_error(JSON_RPC_SERVER_ERROR, "Array is invalid");
                            return fc::json::to_string(response);
                        }
                    } else {
                        return fc::json::to_string(my->rpc(v));
                    }
                } catch (fc::exception &e) {
                    json_rpc_response response;
                    response.error = json_rpc_error(JSON_RPC_SERVER_ERROR, e.to_string(),
                                                    fc::variant(*(e.dynamic_copy_exception())));
                    return fc::json::to_string(response);
                }

            }

            fc::variant plugin::call(const msg_pack &msg) {
                auto new_msg = msg;
                auto parent_plugin = my->get_methods_parent_plugin( msg.method );
                new_msg.plugin = parent_plugin;
                api_method *call = nullptr;
                call = my->find_api_method(new_msg.plugin, new_msg.method);
                fc::variant tmp;
                tmp = (*call)(new_msg);
                return tmp;
            }

        }
    }
} // steem::plugins::json_rpc

FC_REFLECT((golos::plugins::json_rpc::json_rpc_error), (code)(message)(data))
FC_REFLECT((golos::plugins::json_rpc::json_rpc_response), (jsonrpc)(result)(error)(id))

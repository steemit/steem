#include <golos/plugins/json_rpc/plugin.hpp>
#include <golos/plugins/json_rpc/utility.hpp>

#include <boost/algorithm/string.hpp>

#include <fc/log/logger_config.hpp>
#include <fc/exception/exception.hpp>
#include <thirdparty/fc/vendor/websocketpp/websocketpp/error.hpp>

namespace golos {
    namespace plugins {
        namespace json_rpc {
            struct json_rpc_error {
                json_rpc_error() : code(0) {
                }

                json_rpc_error(int32_t c, std::string m, fc::optional<fc::variant> d = fc::optional<fc::variant>())
                        : code(c), message(std::move(m)), data(std::move(d)) {
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

            struct msg_pack::impl final {
                using handler_type = std::function<void (json_rpc_response &)>;

                json_rpc_response response;
                handler_type handler;
            };

            msg_pack::msg_pack() {
            }

            // Constructor with hidden handlers types
            template <typename Handler>
            msg_pack::msg_pack(Handler &&handler): pimpl(new impl) {
                pimpl->handler = std::move(handler);
            }

            // Move constructor/operator move handlers, so original msg_pack can't pass result/error to connection
            msg_pack::msg_pack(msg_pack &&src): pimpl(std::move(src.pimpl)) {
            }

            msg_pack::~msg_pack() = default;

            msg_pack & msg_pack::operator=(msg_pack &&src) {
                pimpl = std::move(src.pimpl);
                return *this;
            }

            bool msg_pack::valid() const {
                return pimpl.get() != nullptr;
            }

            void msg_pack::rpc_id(fc::variant id) {
                // Pimpl can absent in case if msg_pack delegated its handlers to other msg_pack (see move constructor)
                FC_ASSERT(valid(), "The msg_pack delegated its handlers");

                switch (id.get_type()) {
                    case fc::variant::int64_type:
                    case fc::variant::uint64_type:
                    case fc::variant::string_type:
                        pimpl->response.id = std::move(id);
                        break;

                    default:
                        FC_THROW_EXCEPTION(fc::parse_error_exception, "Only integer value or string is allowed for member \"id\"");
                }
            }

            fc::optional<fc::variant> msg_pack::rpc_id() const {
                // Pimpl can absent in case if msg_pack delegated its handlers to other msg_pack (see move constructor)
                if (valid()) {
                    return pimpl->response.id;
                }
                return fc::optional<fc::variant>();
            }

            void msg_pack::unsafe_result(fc::optional<fc::variant> result) {
                // Pimpl can absent in case if msg_pack delegated its handlers to other msg_pack (see move constructor)
                FC_ASSERT(valid(), "The msg_pack delegated its handlers");
                pimpl->response.result = std::move(result);
                pimpl->handler(pimpl->response);
            }

            void msg_pack::result(fc::optional<fc::variant> result) {
                // Pimpl can absent in case if msg_pack delegated its handlers to other msg_pack (see move constructor)
                try {
                    unsafe_result(std::move(result));
                } catch (const websocketpp::exception &) {
                    // Can't send data via socket -
                    //    don't pass exception to upper level, because it doesn't have handler for exception
                }
            }

            fc::optional<fc::variant> msg_pack::result() const {
                // Pimpl can absent in case if msg_pack delegated its handlers to other msg_pack (see move constructor)
                if (valid()) {
                    return pimpl->response.result;
                }
                return fc::optional<fc::variant>();
            }

            void msg_pack::error(int32_t code, std::string message, fc::optional<fc::variant> data) {
                // Pimpl can absent in case if msg_pack delegated its handlers to other msg_pack (see move constructor)
                FC_ASSERT(valid(), "The msg_pack delegated its handlers");
                pimpl->response.error = json_rpc_error(code, std::move(message), std::move(data));
                try {
                    pimpl->handler(pimpl->response);
                } catch (const websocketpp::exception &) {
                    // Can't send data via socket - see
                    //    don't pass exception to upper level, because it doesn't have handler for exception
               }
            }

            void msg_pack::error(std::string message, fc::optional<fc::variant> data) {
                error(JSON_RPC_SERVER_ERROR, std::move(message), std::move(data));
            }

            void msg_pack::error(const fc::exception &e) {
                error(JSON_RPC_SERVER_ERROR, e);
            }

            void msg_pack::error(int32_t code, const fc::exception &e) {
                error(code, e.to_string(), fc::variant(*(e.dynamic_copy_exception())));
            }

            fc::optional<std::string> msg_pack::error() const {
                // Pimpl can absent in case if msg_pack delegated its handlers to other msg_pack (see move constructor)
                if (valid() || pimpl->response.error.valid()) {
                    return pimpl->response.error->message;
                }
                return fc::optional<std::string>();
            }

            using get_methods_args     = void_type;
            using get_methods_return   = vector<string>;
            using get_signature_args   = string;
            using get_signature_return = api_method_signature;

            class plugin::impl final {
            public:
                impl() {
                }

                ~impl() {
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

                void rpc_jsonrpc(const fc::variant_object &request, msg_pack &msg) {
                    // TODO: id is optional value or not?
                    if (request.contains("id")) {
                        msg.rpc_id(request["id"]);
                    }

                    if (!request.contains("jsonrpc") || request["jsonrpc"].as_string() != "2.0") {
                        return msg.error(JSON_RPC_INVALID_REQUEST, "jsonrpc value is not \"2.0\"");
                    } else if (!request.contains("method")) {
                        return msg.error(JSON_RPC_INVALID_REQUEST, "A member \"method\" does not exist");
                    }

                    string method;

                    try {
                        method = request["method"].as_string();
                    } catch (const fc::assert_exception &e) {
                        return msg.error(JSON_RPC_METHOD_NOT_FOUND, e);
                    }

                    // This is to maintain backwards compatibility with existing call structure.
                    if ((method == "call" && request.contains("params")) || method != "call") {
                        api_method *call = nullptr;

                        try {
                            call = process_params(method, request, msg);
                        } catch (const fc::assert_exception &e) {
                            return msg.error(JSON_RPC_PARSE_PARAMS_ERROR, e);
                        }

                        try {
                            auto result = (*call)(msg);
                            if (msg.valid()) {
                                msg.result(std::move(result));
                            }
                        } catch (const fc::assert_exception &e) {
                            return msg.error(JSON_RPC_ERROR_DURING_CALL, e);
                        }
                    } else {
                        return msg.error(JSON_RPC_NO_PARAMS, "A member \"params\" does not exist");
                    }
                }

#define ddump(SEQ) \
                    dlog( FC_FORMAT(SEQ), FC_FORMAT_ARG_PARAMS(SEQ) )

                void rpc(const fc::variant &data, msg_pack &msg) {
                    ddump((data));

                    try {
                        rpc_jsonrpc(data.get_object(), msg);
                    } catch (const fc::parse_error_exception &e) {
                        msg.error(JSON_RPC_INVALID_PARAMS, e);
                    } catch (const fc::bad_cast_exception &e) {
                        msg.error(JSON_RPC_INVALID_PARAMS, e);
                    } catch (const fc::exception &e) {
                        msg.error(e);
                    } catch (...) {
                        msg.error("Unknown error - parsing rpc message failed");
                    }
                }

                void rpc(vector<fc::variant> messages, response_handler_type response_handler) {
                    auto responses = std::make_shared<vector<json_rpc_response>>();

                    responses->reserve(messages.size());

                    std::function<void()> next_handler = [response_handler, responses]{
                        response_handler(fc::json::to_string(*responses.get()));
                    };

                    for (auto it = messages.rbegin(); messages.rend() != it; ++it) {
                        auto v = *it;

                        next_handler = [next_handler, responses, v, this]{
                            msg_pack msg([next_handler, responses](json_rpc_response &response){
                                responses->push_back(response);
                                next_handler();
                            });

                            this->rpc(v, msg);
                        };
                    }

                    next_handler();
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

            plugin::plugin() {
            }

            plugin::~plugin() {
            }

            void plugin::plugin_initialize(const boost::program_options::variables_map &options) {
                ilog("json_rpc plugin: plugin_initialize() begin");
                pimpl = std::make_unique<impl>();
                pimpl->initialize();
                ilog("json_rpc plugin: plugin_initialize() end");
            }

            void plugin::plugin_startup() {
                ilog("json_rpc plugin: plugin_startup() begin");
                std::sort(pimpl->_methods.begin(), pimpl->_methods.end());
                ilog("json_rpc plugin: plugin_startup() end");
            }

            void plugin::plugin_shutdown() {
                ilog("json_rpc plugin: plugin_shutdown() begin");

                ilog("json_rpc plugin: plugin_shutdown() end");
            }

            void plugin::add_api_method(const string &api_name, const string &method_name,
                                        const api_method &api/*, const api_method_signature& sig */) {
                pimpl->add_api_method(api_name, method_name, api/*, sig*/ );
            }

            void plugin::call(const string &message, response_handler_type response_handler) {
                try {
                    fc::variant v = fc::json::from_string(message);

                    if (v.is_array()) {
                        vector<fc::variant> messages = v.as<vector<fc::variant>>();

                        FC_ASSERT(messages.size(), "Array is invalid");
                        pimpl->rpc(messages, response_handler);
                    } else {
                        msg_pack msg([response_handler](json_rpc_response &response){
                            response_handler(fc::json::to_string(response));
                        });

                        pimpl->rpc(v, msg);
                    }
                } catch (const fc::exception &e) {
                    json_rpc_response response;
                    response.error = json_rpc_error(JSON_RPC_SERVER_ERROR, e.to_string(), fc::variant(*(e.dynamic_copy_exception())));
                    response_handler(fc::json::to_string(response));
                }
            }
        }
    }
} // golos::plugins::json_rpc

FC_REFLECT((golos::plugins::json_rpc::json_rpc_error), (code)(message)(data))
FC_REFLECT((golos::plugins::json_rpc::json_rpc_response), (jsonrpc)(result)(error)(id))

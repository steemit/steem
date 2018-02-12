#include <golos/plugins/webserver/webserver_plugin.hpp>

#include <golos/plugins/chain/plugin.hpp>

#include <fc/network/ip.hpp>
#include <fc/log/logger_config.hpp>
#include <fc/io/json.hpp>
#include <fc/network/resolve.hpp>

#include <boost/asio.hpp>
#include <boost/optional.hpp>
#include <boost/bind.hpp>
#include <boost/preprocessor/stringize.hpp>

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/config/asio.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/logger/stub.hpp>
#include <websocketpp/logger/syslog.hpp>

#include <thread>
#include <memory>
#include <iostream>
#include <golos/plugins/json_rpc/plugin.hpp>

namespace golos {
    namespace plugins {
        namespace webserver {

            namespace asio = boost::asio;

            using std::map;
            using std::string;
            using boost::optional;
            using boost::asio::ip::tcp;
            using std::shared_ptr;
            using websocketpp::connection_hdl;

            typedef uint32_t thread_pool_size_t;

            struct asio_with_stub_log : public websocketpp::config::asio {
                typedef asio_with_stub_log type;
                typedef asio base;

                typedef base::concurrency_type concurrency_type;

                typedef base::request_type request_type;
                typedef base::response_type response_type;

                typedef base::message_type message_type;
                typedef base::con_msg_manager_type con_msg_manager_type;
                typedef base::endpoint_msg_manager_type endpoint_msg_manager_type;

                typedef base::alog_type alog_type;
                typedef base::elog_type elog_type;
                //typedef websocketpp::log::stub elog_type;
                //typedef websocketpp::log::stub alog_type;

                typedef base::rng_type rng_type;

                struct transport_config : public base::transport_config {
                    typedef type::concurrency_type concurrency_type;
                    typedef type::alog_type alog_type;
                    typedef type::elog_type elog_type;
                    typedef type::request_type request_type;
                    typedef type::response_type response_type;
                    typedef websocketpp::transport::asio::basic_socket::endpoint socket_type;
                };

                typedef websocketpp::transport::asio::endpoint<transport_config> transport_type;

                static const long timeout_open_handshake = 0;
            };

            using websocket_server_type = websocketpp::server<asio_with_stub_log>;

            struct webserver_plugin::webserver_plugin_impl final {
            public:
                boost::thread_group& thread_pool = appbase::app().scheduler();
                webserver_plugin_impl(thread_pool_size_t thread_pool_size) : thread_pool_work(this->thread_pool_ios) {
                    for (uint32_t i = 0; i < thread_pool_size; ++i) {
                        thread_pool.create_thread(boost::bind(&asio::io_service::run, &thread_pool_ios));
                    }
                }

                void start_webserver();

                void stop_webserver();

                void handle_ws_message(websocket_server_type *, connection_hdl, websocket_server_type::message_ptr);

                void handle_http_message(websocket_server_type *, connection_hdl);

                shared_ptr<std::thread> http_thread;
                asio::io_service http_ios;
                optional<tcp::endpoint> http_endpoint;
                websocket_server_type http_server;

                shared_ptr<std::thread> ws_thread;
                asio::io_service ws_ios;
                optional<tcp::endpoint> ws_endpoint;
                websocket_server_type ws_server;
                asio::io_service thread_pool_ios;
                asio::io_service::work thread_pool_work;

                plugins::json_rpc::plugin *api;
                boost::signals2::connection chain_sync_con;
            };

            void webserver_plugin::webserver_plugin_impl::start_webserver() {
                if (ws_endpoint) {
                    ws_thread = std::make_shared<std::thread>([&]() {
                        ilog("start processing ws thread");
                        try {
                            ws_server.clear_access_channels(websocketpp::log::alevel::all);
                            ws_server.clear_error_channels(websocketpp::log::elevel::all);
                            ws_server.init_asio(&ws_ios);
                            ws_server.set_reuse_addr(true);

                            ws_server.set_message_handler(boost::bind(&webserver_plugin_impl::handle_ws_message, this, &ws_server, _1, _2));

                            if (http_endpoint && http_endpoint == ws_endpoint) {
                                ws_server.set_http_handler(boost::bind(&webserver_plugin_impl::handle_http_message, this, &ws_server, _1));
                                ilog("start listending for http requests");
                            }

                            ilog("start listening for ws requests");
                            ws_server.listen(*ws_endpoint);
                            ws_server.start_accept();

                            ws_ios.run();
                            ilog("ws io service exit");
                        } catch (...) {
                            elog("error thrown from http io service");
                        }
                    });
                }

                if (http_endpoint && ((ws_endpoint && ws_endpoint != http_endpoint) || !ws_endpoint)) {
                    if (1) {
                        ilog("http thread is disabled in this build!");
                    } else {
                        http_thread = std::make_shared<std::thread>([&]() {
                            ilog("start processing http thread");
                            try {
                                http_server.clear_access_channels(websocketpp::log::alevel::all);
                                http_server.clear_error_channels(websocketpp::log::elevel::all);
                                http_server.init_asio(&http_ios);
                                http_server.set_reuse_addr(true);

                                http_server.set_http_handler(
                                        boost::bind(&webserver_plugin_impl::handle_http_message, this, &http_server,
                                                    _1));

                                ilog("start listening for http requests");
                                http_server.listen(*http_endpoint);
                                http_server.start_accept();

                                http_ios.run();
                                ilog("http io service exit");
                            } catch (...) {
                                elog("error thrown from http io service");
                            }
                        });
                    }
                }
            }

            void webserver_plugin::webserver_plugin_impl::stop_webserver() {
                if (ws_server.is_listening()) {
                    ws_server.stop_listening();
                }

                if (http_server.is_listening()) {
                    http_server.stop_listening();
                }

                thread_pool_ios.stop();
                thread_pool.join_all();

                if (ws_thread) {
                    ws_ios.stop();
                    ws_thread->join();
                    ws_thread.reset();
                }

                if (http_thread) {
                    http_ios.stop();
                    http_thread->join();
                    http_thread.reset();
                }
            }

            void webserver_plugin::webserver_plugin_impl::handle_ws_message(websocket_server_type *server,
                                                                            connection_hdl hdl,
                                                                            websocket_server_type::message_ptr msg) {
                auto con = server->get_con_from_hdl(hdl);
                thread_pool_ios.post([con, msg, this]() {
                    try {
                        if (msg->get_opcode() == websocketpp::frame::opcode::text) {
                            api->call(msg->get_payload(), [con](const std::string &data){
                                con->send(data);
                            });
                        } else {
                            con->send("error: string payload expected");
                        }
                    } catch (const fc::exception &e) {
                        con->send("error calling API " + e.to_string());
                    }
                });
            }

            void webserver_plugin::webserver_plugin_impl::handle_http_message(websocket_server_type *server, connection_hdl hdl) {
                auto con = server->get_con_from_hdl(hdl);
                con->defer_http_response();

                thread_pool_ios.post([con, this]() {
                    auto body = con->get_request_body();

                    try {
                        api->call(body, [con](const std::string &data){
                            // this lambda can be called from any thread in application
                            //   for example, when task was delegated ( see msg_pack(msg_pack&&) )
                            con->set_body(data);
                            con->set_status(websocketpp::http::status_code::ok);
                            con->send_http_response();
                        });
                    } catch (fc::exception &e) {
                        // this case happens if exception was thrown on parsing request
                        edump((e));
                        con->set_body("Could not call API");
                        con->set_status(websocketpp::http::status_code::not_found);
                        // this sending response can't be merged with sending response from try-block
                        //   because try-block can work from other thread,
                        //   when catch-block happens in current thread on parsing request
                        con->send_http_response();
                    }
                });
            }

            webserver_plugin::webserver_plugin() {
            }

            webserver_plugin::~webserver_plugin() {
            }

            void webserver_plugin::set_program_options(boost::program_options::options_description &, boost::program_options::options_description &cfg) {
                cfg.add_options()("webserver-http-endpoint", boost::program_options::value<string>(),
                                  "Local http endpoint for webserver requests.")("webserver-ws-endpoint",
                                                                                 boost::program_options::value<string>(),
                                                                                 "Local websocket endpoint for webserver requests.")(
                        "rpc-endpoint", boost::program_options::value<string>(),
                        "Local http and websocket endpoint for webserver requests. Deprectaed in favor of webserver-http-endpoint and webserver-ws-endpoint")(
                        "webserver-thread-pool-size", boost::program_options::value<thread_pool_size_t>()->default_value(256),
                        "Number of threads used to handle queries. Default: 256.");
            }

            void webserver_plugin::plugin_initialize(const boost::program_options::variables_map &options) {
                auto thread_pool_size = options.at("webserver-thread-pool-size").as<thread_pool_size_t>();
                FC_ASSERT(thread_pool_size > 0, "webserver-thread-pool-size must be greater than 0");
                ilog("configured with ${tps} thread pool size", ("tps", thread_pool_size));
                my.reset(new webserver_plugin_impl(thread_pool_size));

                if (options.count("webserver-http-endpoint")) {
                    auto http_endpoint = options.at("webserver-http-endpoint").as<string>();
                    auto endpoints = appbase::app().resolve_string_to_ip_endpoints(http_endpoint);
                    FC_ASSERT(endpoints.size(), "webserver-http-endpoint ${hostname} did not resolve",
                              ("hostname", http_endpoint));
                    my->http_endpoint = endpoints[0];
                    auto tcp_endpoint = endpoints[0];
                    auto ip_port = tcp_endpoint.address().to_string() + ":" + std::to_string(tcp_endpoint.port());
                    ilog("configured http to listen on ${ep}", ("ep", ip_port));
                }

                if (options.count("webserver-ws-endpoint")) {
                    auto ws_endpoint = options.at("webserver-ws-endpoint").as<string>();
                    auto endpoints = appbase::app().resolve_string_to_ip_endpoints(ws_endpoint);
                    FC_ASSERT(endpoints.size(), "ws-server-endpoint ${hostname} did not resolve",
                              ("hostname", ws_endpoint));
                    my->ws_endpoint = endpoints[0];
                    auto tcp_endpoint = endpoints[0];
                    auto ip_port = tcp_endpoint.address().to_string() + ":" + std::to_string(tcp_endpoint.port());
                    ilog("configured ws to listen on ${ep}", ("ep", ip_port));
                }

                if (options.count("rpc-endpoint")) {
                    auto endpoint = options.at("rpc-endpoint").as<string>();
                    auto endpoints = appbase::app().resolve_string_to_ip_endpoints(endpoint);
                    FC_ASSERT(endpoints.size(), "rpc-endpoint ${hostname} did not resolve", ("hostname", endpoint));

                    auto tcp_endpoint = endpoints[0];
                    auto ip_port = tcp_endpoint.address().to_string() + ":" + std::to_string(tcp_endpoint.port());

                    if (!my->http_endpoint) {
                        my->http_endpoint = tcp_endpoint;
                        ilog("configured http to listen on ${ep}", ("ep", ip_port));
                    }

                    if (!my->ws_endpoint) {
                        my->ws_endpoint = tcp_endpoint;
                        ilog("configured ws to listen on ${ep}", ("ep", ip_port));
                    }
                }
            }

            void webserver_plugin::plugin_startup() {
                my->api = appbase::app().find_plugin<plugins::json_rpc::plugin>();
                FC_ASSERT(my->api != nullptr, "Could not find API Register Plugin");

                chain::plugin *chain = appbase::app().find_plugin<chain::plugin>();
                if (chain != nullptr && chain->get_state() != appbase::abstract_plugin::started) {
                    ilog("Waiting for chain plugin to start");
                    my->chain_sync_con = chain->on_sync.connect([this]() {
                        my->start_webserver();
                    });
                } else {
                    my->start_webserver();
                }
            }

            void webserver_plugin::plugin_shutdown() {
                my->stop_webserver();
            }

        }
    }
} // steem::plugins::webserver

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <iterator>

#include <fc/io/json.hpp>
#include <fc/io/stdio.hpp>
#include <fc/network/http/server.hpp>
#include <fc/network/http/websocket.hpp>
#include <fc/rpc/cli.hpp>
#include <fc/rpc/http_api.hpp>
#include <fc/rpc/websocket_api.hpp>
#include <fc/smart_ref_impl.hpp>
#include <fc/log/console_appender.hpp>
#include <fc/log/file_appender.hpp>
#include <fc/log/logger.hpp>
#include <fc/log/logger_config.hpp>
#include <fc/interprocess/signals.hpp>
#include <fc/variant.hpp>

#include <graphene/utilities/key_conversion.hpp>

#include <golos/protocol/protocol.hpp>
#include <golos/wallet/remote_node_api.hpp>
#include <golos/wallet/wallet.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include <map>


#ifdef WIN32
# include <signal.h>
#else
# include <csignal>
#endif


using namespace golos::utilities;
using namespace golos::chain;
using namespace golos::wallet;
using namespace std;

void daemon_mode();

void non_daemon_mode(
    const bpo::variables_map& options,
    const vector<string>& commands,
    const bool interactive,
    std::shared_ptr<fc::rpc::cli> wallet_cli,
    const fc::api<wallet_api>& wapi
);

void parse_commands(
    const bpo::variables_map& options,
    vector<string>& commands,
    bool& interactive
);

// Sets both default value and implicit (to preserve `--option=param` syntax)
#define BPO_VALUE_DEFAULT(T, v) (bpo::value<T>()->default_value(v)->implicit_value(v))

int unsafe_main(int argc, char** argv);

int main(int argc, char** argv) {
    try {
        return unsafe_main(argc, argv);
    } catch (const fc::exception& e) {
        cout << e.to_detail_string() << "\n";
        return -1;
    }
}

int unsafe_main(int argc, char** argv) {
    boost::program_options::options_description opts;
    opts.add_options()
            ("help,h", "Print this help message and exit.")
            ("server-rpc-endpoint,s", bpo::value<string>()->implicit_value("ws://127.0.0.1:8090"), "Server websocket RPC endpoint")
            ("cert-authority,a", BPO_VALUE_DEFAULT(string, "_default"), "Trusted CA bundle file for connecting to wss:// TLS server")
            ("rpc-endpoint,r", bpo::value<string>()->implicit_value("127.0.0.1:8091"), "Endpoint for wallet websocket RPC to listen on")
            ("rpc-tls-endpoint,t", bpo::value<string>()->implicit_value("127.0.0.1:8092"), "Endpoint for wallet websocket TLS RPC to listen on")
            ("rpc-tls-certificate,c", BPO_VALUE_DEFAULT(string, "server.pem"), "PEM certificate for wallet websocket TLS RPC")
            ("rpc-http-endpoint,H", bpo::value<string>()->implicit_value("127.0.0.1:8093"), "Endpoint for wallet HTTP RPC to listen on")
            ("daemon,d", "Run the wallet in daemon mode" )
            ("rpc-http-allowip", bpo::value<vector<string>>()->multitoken(), "Allows only specified IPs to connect to the HTTP endpoint" )
            ("wallet-file,w", BPO_VALUE_DEFAULT(string, "wallet.json"), "wallet to load")
#ifdef IS_TEST_NET
        ("chain-id", BPO_VALUE_DEFAULT(string, STEEM_CHAIN_ID_NAME), "chain ID to connect to")
#endif
            ("commands,C", bpo::value<string>(), "Enable non-interactive mode")
            ;

    bpo::options_description log_opts("Logging options");
    log_opts.add_options()
        ("logger.default.level", BPO_VALUE_DEFAULT(string, "info"), "Log level of \"default\" (stderr) logger (all|debug|info|warn|error|off)")
        ("logger.rpc.level", BPO_VALUE_DEFAULT(string, "debug"), "Log level of \"rpc\" logger (all|debug|info|warn|error|off)")
        ("logger.rpc.filename", BPO_VALUE_DEFAULT(string, "logs/rpc/rpc.log"), "rpc logger filename, can be absolute or relative")
        ("logger.rpc.flush", BPO_VALUE_DEFAULT(bool, true), "rpc logger flush (true|false or 1|0)")
        ("logger.rpc.rotate", BPO_VALUE_DEFAULT(bool, true), "rpc logger rotate (true|false or 1|0)")
        ("logger.rpc.rotation_interval", BPO_VALUE_DEFAULT(int64_t, fc::hours(1).to_seconds()), "rpc logger rotation interval in seconds")
        ("logger.rpc.rotation_limit", BPO_VALUE_DEFAULT(int64_t, fc::days(1).to_seconds()), "rpc logger rotation limit in seconds")
        ;
    opts.add(log_opts);

    bpo::variables_map options;
    bpo::store(bpo::parse_command_line(argc, argv, opts), options);

    if (options.count("help")) {
        cout << opts << "\n";
        return 0;
    }

    vector<string> commands;
    bool interactive = true;
    parse_commands(options, commands, interactive);

    golos::protocol::chain_id_type _steem_chain_id = STEEMIT_CHAIN_ID;

    // Note: each logging option have default value, no need to check options.count()
    auto ll_default = fc::variant(options["logger.default.level"].as<string>()).as<fc::log_level>();
    auto ll_rpc = fc::variant(options["logger.rpc.level"].as<string>()).as<fc::log_level>();
    
    fc::logging_config cfg;
    fc::file_appender::config ac;
    if (ll_rpc < fc::log_level::off) {
        ac.filename             = options["logger.rpc.filename"].as<string>();
        ac.flush                = options["logger.rpc.flush"].as<bool>();
        ac.rotate               = options["logger.rpc.rotate"].as<bool>();
        ac.rotation_interval    = fc::seconds(options["logger.rpc.rotation_interval"].as<int64_t>());
        ac.rotation_limit       = fc::seconds(options["logger.rpc.rotation_limit"].as<int64_t>());

        cout << "Logging RPC to file: " << (ac.filename).preferred_string() << "\n";
    }

    cfg.appenders.push_back(fc::appender_config("default", "console", fc::variant(fc::console_appender::config())));
    cfg.appenders.push_back(fc::appender_config("rpc", "file", fc::variant(ac)));

    cfg.loggers = { fc::logger_config("default"), fc::logger_config( "rpc") };
    cfg.loggers.front().level = ll_default;
    cfg.loggers.front().appenders = {"default"};
    cfg.loggers.back().level = ll_rpc;
    cfg.loggers.back().appenders = {"rpc"};
    fc::configure_logging(cfg);

    //
    // TODO:  We read wallet_data twice, once in main() to grab the
    //    socket info, again in wallet_api when we do
    //    load_wallet_file().  Seems like this could be better
    //    designed.
    //
    wallet_data wdata;
    fc::path wallet_file(options["wallet-file"].as<string>());
    if (fc::exists(wallet_file)) {
        wdata = fc::json::from_file(wallet_file).as<wallet_data>();
    } else {
        cout << "Starting a new wallet\n";
    }

    // but allow CLI to override
    if (options.count("server-rpc-endpoint"))
        wdata.ws_server = options["server-rpc-endpoint"].as<string>();

    fc::http::websocket_client client(options["cert-authority"].as<string>());
    idump((wdata.ws_server));
    auto con  = client.connect(wdata.ws_server);
    auto apic = std::make_shared<fc::rpc::websocket_api_connection>(*con);

    auto wapiptr = std::make_shared<wallet_api>(wdata, _steem_chain_id, *apic);
    wapiptr->set_wallet_filename(wallet_file.generic_string());
    wapiptr->load_wallet_file();

    fc::api<wallet_api> wapi(wapiptr);

    auto wallet_cli = std::make_shared<fc::rpc::cli>();
    for (auto& name_formatter : wapiptr->get_result_formatters())
        wallet_cli->format_result(name_formatter.first, name_formatter.second);

    boost::signals2::scoped_connection closed_connection(con->closed.connect([=,&client]{
        cerr << "Server has disconnected us, reconnecting.\n";
        client.connect(wdata.ws_server, true);
    }));
    (void)(closed_connection);

    if( wapiptr->is_new() ) {
        cout << "Please use the set_password method to initialize a new wallet before continuing\n";
        wallet_cli->set_prompt( "new >>> " );
    } else
        wallet_cli->set_prompt( "locked >>> " );

    boost::signals2::scoped_connection locked_connection(wapiptr->lock_changed.connect([&](bool locked) {
        wallet_cli->set_prompt(  locked ? "locked >>> " : "unlocked >>> " );
    }));

    auto _websocket_server = std::make_shared<fc::http::websocket_server>();
    if (options.count("rpc-endpoint")) {
        _websocket_server->on_connection([&](const fc::http::websocket_connection_ptr& c){
            cout << "here... \n";
            wlog(".");
            auto wsc = std::make_shared<fc::rpc::websocket_api_connection>(*c);
            wsc->register_api(wapi);
            c->set_session_data(wsc);
        });
        ilog("Listening for incoming RPC requests on ${p}", ("p", options["rpc-endpoint"].as<string>()));
        _websocket_server->listen(fc::ip::endpoint::from_string(options["rpc-endpoint"].as<string>()));
        _websocket_server->start_accept();
    }

    string cert_pem = options["rpc-tls-certificate"].as<string>();
    auto _websocket_tls_server = std::make_shared<fc::http::websocket_tls_server>(cert_pem);
    if (options.count("rpc-tls-endpoint")) {
        _websocket_tls_server->on_connection([&](const fc::http::websocket_connection_ptr& c){
            auto wsc = std::make_shared<fc::rpc::websocket_api_connection>(*c);
            wsc->register_api(wapi);
            c->set_session_data(wsc);
        });
        ilog("Listening for incoming TLS RPC requests on ${p}", ("p", options["rpc-tls-endpoint"].as<string>()));
        _websocket_tls_server->listen(fc::ip::endpoint::from_string(options["rpc-tls-endpoint"].as<string>()));
        _websocket_tls_server->start_accept();
    }

    set<fc::ip::address> allowed_ip_set;

    auto _http_server = std::make_shared<fc::http::server>();
    if (options.count("rpc-http-endpoint")) {
        auto rpc_http_endpoint = options["rpc-http-endpoint"].as<string>();
        ilog("Listening for incoming HTTP RPC requests on ${p}", ("p", rpc_http_endpoint));

        if (options.count("rpc-http-allowip")) {
            auto allowed_ips = options["rpc-http-allowip"].as<vector<string>>();
            wdump((allowed_ips));
            for (const auto& ip : allowed_ips)
                allowed_ip_set.insert(fc::ip::address(ip));
        }

        _http_server->listen(fc::ip::endpoint::from_string(rpc_http_endpoint));
        //
        // due to implementation, on_request() must come AFTER listen()
        //
        _http_server->on_request(
                [&](const fc::http::request& req, const fc::http::server::response& resp) {
                    auto itr = allowed_ip_set.find( fc::ip::endpoint::from_string(req.remote_endpoint).get_address() );
                    if( itr == allowed_ip_set.end() ) {
                        elog("rejected connection from ${ip} because it isn't in allowed set ${s}", ("ip",req.remote_endpoint)("s",allowed_ip_set) );
                        resp.set_status( fc::http::reply::NotAuthorized );
                        return;
                    }
                    std::shared_ptr<fc::rpc::http_api_connection> conn =
                            std::make_shared<fc::rpc::http_api_connection>();
                    conn->register_api(wapi);
                    conn->on_request(req, resp);
                } );
    }

    if (options.count("daemon")) {
        daemon_mode();
    } else {
        non_daemon_mode(options, commands, interactive, wallet_cli, wapi);
    }

    wapi->save_wallet_file(wallet_file.generic_string());
    locked_connection.disconnect();
    closed_connection.disconnect();

    return 0;
}

void non_daemon_mode (
    const bpo::variables_map& options,
    const vector<string>& commands,
    const bool interactive,
    std::shared_ptr<fc::rpc::cli> wallet_cli,
    const fc::api<wallet_api>& wapi
) {
    wallet_cli->register_api(wapi);
    if (interactive) {
        wallet_cli->start();
        wallet_cli->wait();
    } else {
        vector<pair<string,string>> commands_output;
        for (auto const &command : commands) {
            try {
                auto result = wallet_cli->exec_command ( command );
                commands_output.push_back ({command, result}) ;
            }
            catch ( const fc::exception& e ) {
                cout << e.to_detail_string() << '\n';
            }
        }
        for (auto i : commands_output) {
            cout << i.first << '\n' << fc::json::to_pretty_string( i.second ) << '\n';
        }
    }
}

void daemon_mode() {
    fc::promise<int>::ptr exit_promise = new fc::promise<int>("UNIX Signal Handler");
    fc::set_signal_handler([&exit_promise](int signal) {
        exit_promise->set_value(signal);
    }, SIGINT);

    ilog("Entering Daemon Mode, ^C to exit");
    exit_promise->wait();
}

void parse_commands(
    const bpo::variables_map& options,
    vector<string>& commands,
    bool& interactive
) {
    if (options.count("commands")) {
        // If you would like to enable non-interactive mode, then you should
        // pass commands you like cli_wallet to execute via 'commands' program
        // option. All commands should be separated with "&&". The order does matter!
        // EXAMPLE: ./cli_wallet --commands="unlock verystrongpassword && some_command arg1 arg2 && another_command arg1 arg2 arg3"

        interactive = false;
        auto tmp_commmad_string = options["commands"].as<string>();

        // Here will be stored the strings that will be parsed by the "&&"
        vector<string> unchecked_commands;
        auto delims = "&&";

        boost::algorithm::split_regex(unchecked_commands, tmp_commmad_string, boost::regex(delims));

        for (auto x : unchecked_commands) {
            boost::trim(x);
            if (x != "") {
                commands.push_back(x);
            }
        }
    }
}

#include <appbase/application.hpp>
#include <golos/protocol/types.hpp>
#include <fc/log/logger_config.hpp>
#include <graphene/utilities/key_conversion.hpp>
#include <fc/git_revision.hpp>
#include <golos/plugins/chain/plugin.hpp>
#include <golos/plugins/p2p/p2p_plugin.hpp>
#include <golos/plugins/webserver/webserver_plugin.hpp>
#include <golos/plugins/network_broadcast_api/network_broadcast_api_plugin.hpp>
#include <golos/plugins/tags/tags_plugin.hpp>
#include <golos/plugins/witness/witness.hpp>
#include <golos/plugins/database_api/plugin.hpp>
#include <golos/plugins/test_api/test_api_plugin.hpp>
#include <golos/plugins/tolstoy_api/tolstoy_api_plugin.hpp>


#include <fc/exception/exception.hpp>
#include <fc/interprocess/signals.hpp>
#include <fc/git_revision.hpp>

#include <boost/exception/diagnostic_information.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <csignal>
#include <vector>
#include <fc/log/console_appender.hpp>
#include <fc/log/json_console_appender.hpp>
#include <fc/log/file_appender.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string.hpp>

using golos::protocol::version;


std::string& version_string() {
    static std::string v_str =
            "steem_blockchain_version: " + std::string( STEEMIT_BLOCKCHAIN_VERSION ) + "\n" +
            "steem_git_revision:       " + std::string( fc::git_revision_sha )       + "\n" +
            "fc_git_revision:          " + std::string( fc::git_revision_sha )       + "\n";
    return v_str;
}

namespace golos {

    namespace utilities {
        void set_logging_program_options(boost::program_options::options_description &);
        fc::optional<fc::logging_config> load_logging_config( const boost::program_options::variables_map&, const boost::filesystem::path&);

        struct console_appender_args {
            std::string appender;
            std::string stream;
        };

        struct file_appender_args {
            std::string appender;
            std::string file;
        };

        struct logger_args {
            std::string name;
            std::string level;
            std::string appender;
        };
    }

    namespace plugins {
        void register_plugins();
    }

}

void logo(){

#ifdef BUILD_GOLOS_TESTNET
    std::cerr << "------------------------------------------------------\n\n";
        std::cerr << "            STARTING TEST NETWORK\n\n";
        std::cerr << "------------------------------------------------------\n";
        auto initminer_private_key = golos::utilities::key_to_wif( STEEMIT_INIT_PRIVATE_KEY );
        std::cerr << "initminer public key: " << STEEMIT_INIT_PUBLIC_KEY_STR << "\n";
        std::cerr << "initminer private key: " << initminer_private_key << "\n";
        std::cerr << "chain id: " << std::string( STEEMIT_CHAIN_ID ) << "\n";
        std::cerr << "blockchain version: " << std::string( STEEMIT_BLOCKCHAIN_VERSION ) << "\n";
        std::cerr << "------------------------------------------------------\n";
#else
    std::cerr << "------------------------------------------------------\n\n";
    std::cerr << "            STARTING GOLOS NETWORK\n\n";
    std::cerr << "------------------------------------------------------\n";
    std::cerr << "initminer public key: " << STEEMIT_INIT_PUBLIC_KEY_STR << "\n";
    std::cerr << "chain id: " << std::string( STEEMIT_CHAIN_ID ) << "\n";
    std::cerr << "blockchain version: " << std::string( STEEMIT_BLOCKCHAIN_VERSION ) << "\n";
    std::cerr << "------------------------------------------------------\n";
#endif

}

int main( int argc, char** argv ) {
    try {

        // Setup logging config
        boost::program_options::options_description options;

        golos::utilities::set_logging_program_options( options );
        appbase::app().add_program_options( boost::program_options::options_description(), options );

        golos::plugins::register_plugins();
        appbase::app().set_version_string( version_string() );

        bool initialized = appbase::app().initialize<
                golos::plugins::chain::plugin,
                golos::plugins::p2p::p2p_plugin,
                golos::plugins::webserver::webserver_plugin
        >
                ( argc, argv );

        logo();

        if( !initialized )
            return 0;

        auto& args = appbase::app().get_args();

        try {
            fc::optional< fc::logging_config > logging_config = golos::utilities::load_logging_config( args, appbase::app().data_dir() );
            if( logging_config )
                fc::configure_logging( *logging_config );
        } catch( const fc::exception& ) {
            wlog( "Error parsing logging config" );
        }

        appbase::app().startup();
        appbase::app().exec();
        ilog("exited cleanly");
        return 0;
    }
    catch ( const boost::exception& e ) {
        std::cerr << boost::diagnostic_information(e) << "\n";
    }
    catch ( const fc::exception& e ) {
        std::cerr << e.to_detail_string() << "\n";
    }
    catch ( const std::exception& e ) {
        std::cerr << e.what() << "\n";
    }
    catch ( ... ) {
        std::cerr << "unknown exception\n";
    }

    return -1;
}



namespace golos {
    namespace utilities{
        using std::string;
        using std::vector;

        void set_logging_program_options( boost::program_options::options_description& options ) {
            std::vector< std::string > default_console_appender( { "{\"appender\":\"stderr\",\"stream\":\"std_error\"}" } );
            std::string str_default_console_appender = boost::algorithm::join( default_console_appender, " " );

            std::vector< std::string > default_file_appender( { "{\"appender\":\"p2p\",\"file\":\"logs/p2p/p2p.log\"}" } );
            std::string str_default_file_appender = boost::algorithm::join( default_file_appender, " " );

            std::vector< std::string > default_logger(
                    { "{\"name\":\"default\",\"level\":\"warn\",\"appender\":\"stderr\"}\n",
                      "log-logger = {\"name\":\"p2p\",\"level\":\"warn\",\"appender\":\"p2p\"}" } );
            std::string str_default_logger = boost::algorithm::join( default_logger, " " );

            options.add_options()
                    ("log-console-appender", boost::program_options::value< std::vector< std::string > >()->composing()->default_value( default_console_appender, str_default_console_appender ),
                     "Console appender definition json: {\"appender\", \"stream\"}" )
                    ("log-file-appender", boost::program_options::value< std::vector< std::string > >()->composing()->default_value( default_file_appender, str_default_file_appender ),
                     "File appender definition json:  {\"appender\", \"file\"}" )
                    ("log-logger", boost::program_options::value< std::vector< std::string > >()->composing()->default_value( default_logger, str_default_logger ),
                     "Logger definition json: {\"name\", \"level\", \"appender\"}" );
        }

        fc::optional<fc::logging_config> load_logging_config( const boost::program_options::variables_map& args, const boost::filesystem::path& pwd ) {
            try {
                fc::logging_config logging_config;
                bool found_logging_config = false;

                if( args.count( "log-console-appender" ) ) {
                    std::vector< string > console_appenders = args["log-console-appender"].as< vector< string > >();

                    for( string& s : console_appenders ) {
                        try {
                            auto console_appender = fc::json::from_string( s ).as< utilities::console_appender_args >();
                            fc::console_appender::config console_appender_config;
                            console_appender_config.level_colors.emplace_back(fc::console_appender::level_color( fc::log_level::debug, fc::console_appender::color::green));
                            console_appender_config.level_colors.emplace_back(fc::console_appender::level_color( fc::log_level::warn, fc::console_appender::color::brown));
                            console_appender_config.level_colors.emplace_back(fc::console_appender::level_color( fc::log_level::error, fc::console_appender::color::red));
                            console_appender_config.stream = fc::variant( console_appender.stream ).as< fc::console_appender::stream::type >();
                            logging_config.appenders.push_back(fc::appender_config( console_appender.appender, "console", fc::variant( console_appender_config ) ) );
                            found_logging_config = true;
                        }

                        catch( ... ) {

                        }
                    }
                }

                if( args.count( "log-file-appender" ) ) {
                    std::vector< string > file_appenders = args["log-file-appender"].as< std::vector< string > >();

                    for( string& s : file_appenders ) {
                        auto file_appender = fc::json::from_string( s ).as< golos::utilities::file_appender_args>();

                        fc::path file_name = file_appender.file;
                        if( file_name.is_relative() )
                            file_name = fc::absolute( pwd ) / file_name;

                        // construct a default file appender config here
                        // filename will be taken from ini file, everything else hard-coded here
                        fc::file_appender::config file_appender_config;
                        file_appender_config.filename = file_name;
                        file_appender_config.flush = true;
                        file_appender_config.rotate = true;
                        file_appender_config.rotation_interval = fc::hours(1);
                        file_appender_config.rotation_limit = fc::days(1);
                        logging_config.appenders.push_back(
                                fc::appender_config( file_appender.appender, "file", fc::variant( file_appender_config ) ) );
                        found_logging_config = true;
                    }
                }

                if( args.count( "log-logger" ) ) {
                    std::vector< string > loggers = args[ "log-logger" ].as< std::vector< std::string > >();

                    for( string& s : loggers ) {
                        auto logger = fc::json::from_string( s ).as< golos::utilities::logger_args >();

                        fc::logger_config logger_config( logger.name );
                        logger_config.level = fc::variant( logger.level ).as< fc::log_level >();
                        boost::split( logger_config.appenders, logger.appender, boost::is_any_of(" ,"), boost::token_compress_on );
                        logging_config.loggers.push_back( logger_config );
                        found_logging_config = true;
                    }
                }

                if( found_logging_config )
                    return logging_config;
                else
                    return fc::optional< fc::logging_config >();
            }
            FC_RETHROW_EXCEPTIONS(warn, "")
        }
    }

    namespace plugins {
        void register_plugins() {
            appbase::app().register_plugin< golos::plugins::chain::plugin                                       >();
            appbase::app().register_plugin< golos::plugins::p2p::p2p_plugin                                     >();
            appbase::app().register_plugin< golos::plugins::webserver::webserver_plugin                         >();
            appbase::app().register_plugin< golos::plugins::tags::tags_plugin                                   >();
            appbase::app().register_plugin< golos::plugins::witness_plugin::witness_plugin                      >();
            appbase::app().register_plugin< golos::plugins::network_broadcast_api::network_broadcast_api_plugin >();
            golos::plugins::database_api::register_database_api();
            appbase::app().register_plugin< golos::plugins::test_api::test_api_plugin                           >();
            appbase::app().register_plugin< golos::plugins::tolstoy_api::tolstoy_api_plugin                     >();
        }
    }
}

FC_REFLECT( (golos::utilities::console_appender_args), (appender)(stream) )
FC_REFLECT( (golos::utilities::file_appender_args), (appender)(file) )
FC_REFLECT( (golos::utilities::logger_args), (name)(level)(appender) )



/*#include <steemit/app/application.hpp>

#include <steemit/witness/witness.hpp>
#include <steemit/manifest/plugins.hpp>

#include <fc/interprocess/signals.hpp>
#include <fc/log/console_appender.hpp>
#include <fc/log/file_appender.hpp>
#include <fc/log/logger_config.hpp>

#include <graphene/utilities/git_revision.hpp>
#include <fc/git_revision.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#ifdef WIN32
# include <signal.h>
#else

# include <csignal>

#endif

#include <graphene/utilities/key_conversion.hpp>

using namespace golos;
using golos::protocol::version;
namespace bpo = boost::program_options;

void write_default_logging_config_to_stream(std::ostream &out);

fc::optional<fc::logging_config> load_logging_config_from_ini_file(const fc::path &config_ini_filename);

int main(int argc, char **argv) {
    golos::plugin::initialize_plugin_factories();
    app::application *node = new app::application();
    fc::oexception unhandled_exception;
    try {

#ifdef STEEMIT_BUILD_TESTNET
        std::cerr
                << "------------------------------------------------------\n\n";
        std::cerr << "            STARTING TEST NETWORK\n\n";
        std::cerr << "------------------------------------------------------\n";
        auto initminer_private_key = golos::utilities::key_to_wif(STEEMIT_INIT_PRIVATE_KEY);
        std::cerr << "initminer public key: " << STEEMIT_INIT_PUBLIC_KEY_STR
                  << "\n";
        std::cerr << "initminer private key: " << initminer_private_key << "\n";
        std::cerr << "chain id: " << std::string(STEEMIT_CHAIN_ID) << "\n";
        std::cerr << "blockchain version: "
                  << fc::string(STEEMIT_BLOCKCHAIN_VERSION) << "\n";
        std::cerr << "------------------------------------------------------\n";
#else
        std::cerr
                << "------------------------------------------------------\n\n";
        std::cerr << "            STARTING GOLOS NETWORK\n\n";
        std::cerr << "------------------------------------------------------\n";
        std::cerr << "initminer public key: " << STEEMIT_INIT_PUBLIC_KEY_STR
                  << "\n";
        std::cerr << "chain id: " << std::string(STEEMIT_CHAIN_ID) << "\n";
        std::cerr << "blockchain version: "
                  << std::string(STEEMIT_BLOCKCHAIN_VERSION) << "\n";
        std::cerr << "------------------------------------------------------\n";
#endif

        bpo::options_description app_options("Golos Daemon");
        bpo::options_description cfg_options("Golos Daemon");
        app_options.add_options()
                ("help,h", "Print this help message and exit.")
                ("data-dir,d", bpo::value<boost::filesystem::path>()->default_value("witness_node_data_dir"), "Directory containing databases, configuration file, etc.")
                ("version,v", "Print golosd version and exit.");

        bpo::variables_map options;

        for (const std::string &plugin_name : steemit::plugin::get_available_plugins()) {
            node->register_abstract_plugin(steemit::plugin::create_plugin(plugin_name, node));
        }

        try {
            bpo::options_description cli, cfg;
            node->set_program_options(cli, cfg);
            app_options.add(cli);
            cfg_options.add(cfg);
            bpo::store(bpo::parse_command_line(argc, argv, app_options), options);
        }
        catch (const boost::program_options::error &e) {
            std::cerr << "Error parsing command line: " << e.what() << "\n";
            return 1;
        }

        if (options.count("version")) {
            std::cout << "steem_blockchain_version: "
                      << fc::string(STEEMIT_BLOCKCHAIN_VERSION) << "\n";
            std::cout << "steem_git_revision:       "
                      << fc::string(golos::utilities::git_revision_sha)
                      << "\n";
            std::cout << "fc_git_revision:          "
                      << fc::string(fc::git_revision_sha) << "\n";
            return 0;
        }

        if (options.count("help")) {
            std::cout << app_options << "\n";
            return 0;
        }

        fc::path data_dir;
        if (options.count("data-dir")) {
            data_dir = options["data-dir"].as<boost::filesystem::path>();
            if (data_dir.is_relative()) {
                data_dir = fc::current_path() / data_dir;
            }
        }

        fc::path config_ini_path = data_dir / "config.ini";
        if (fc::exists(config_ini_path)) {
            // get the basic options
            bpo::store(bpo::parse_config_file<char>(config_ini_path.preferred_string().c_str(), cfg_options, true), options);

            // try to get logging options from the config file.
            try {
                fc::optional<fc::logging_config> logging_config = load_logging_config_from_ini_file(config_ini_path);
                if (logging_config) {
                    fc::configure_logging(*logging_config);
                }
            }
            catch (const fc::exception &) {
                wlog("Error parsing logging config from config file ${config}, using default config", ("config", config_ini_path.preferred_string()));
            }
        } else {
            ilog("Writing new config file at ${path}", ("path", config_ini_path));
            if (!fc::exists(data_dir)) {
                fc::create_directories(data_dir);
            }

            std::ofstream out_cfg(config_ini_path.preferred_string());
            for (const boost::shared_ptr<bpo::option_description> od : cfg_options.options()) {
                if (!od->description().empty()) {
                    out_cfg << "# " << od->description() << "\n";
                }
                boost::any store;
                if (!od->semantic()->apply_default(store)) {
                    out_cfg << "# " << od->long_name() << " = \n";
                } else {
                    auto example = od->format_parameter();
                    if (example.empty()) {
                        // This is a boolean switch
                        out_cfg << od->long_name() << " = " << "false\n";
                    } else {
                        // The string is formatted "arg (=<interesting part>)"
                        example.erase(0, 6);
                        example.erase(example.length() - 1);
                        out_cfg << od->long_name() << " = " << example << "\n";
                    }
                }
                out_cfg << "\n";
            }
            write_default_logging_config_to_stream(out_cfg);
            out_cfg.close();
            // read the default logging config we just wrote out to the file and start using it
            fc::optional<fc::logging_config> logging_config = load_logging_config_from_ini_file(config_ini_path);
            if (logging_config) {
                fc::configure_logging(*logging_config);
            }
        }

        ilog("parsing options");
        bpo::notify(options);
        ilog("initializing node");
        node->initialize(data_dir, options);
        ilog("initializing plugins");
        node->initialize_plugins(options);

        ilog("starting node");
        node->startup();
        ilog("starting plugins");
        node->startup_plugins();

        fc::promise<int>::ptr exit_promise = new fc::promise<int>("UNIX Signal Handler");

        fc::set_signal_handler([&exit_promise](int signal) {
            elog("Caught SIGINT attempting to exit cleanly");
            exit_promise->set_value(signal);
        }, SIGINT);

        fc::set_signal_handler([&exit_promise](int signal) {
            elog("Caught SIGTERM attempting to exit cleanly");
            exit_promise->set_value(signal);
        }, SIGTERM);

        node->chain_database()->with_read_lock([&]() {
            ilog("Started witness node on a chain with ${h} blocks.", ("h", node->chain_database()->head_block_num()));
        });

        exit_promise->wait();
        node->shutdown_plugins();
        node->shutdown();
        delete node;
        return 0;
    } catch (const fc::exception &e) {
        // deleting the node can yield, so do this outside the exception handler
        unhandled_exception = e;
    }

    if (unhandled_exception) {
        elog("Exiting with error:\n${e}", ("e", unhandled_exception->to_detail_string()));
        node->shutdown();
        delete node;
        return 1;
    }
    ilog("done");
    return 0;
}
*/
// logging config is too complicated to be parsed by boost::program_options,
// so we do it by hand
//
// Currently, you can only specify the filenames and logging levels, which
// are all most users would want to change.  At a later time, options can
// be added to control rotation intervals, compression, and other seldom-
// used features
/*
void write_default_logging_config_to_stream(std::ostream &out) {
    out
            << "# declare an appender named \"stderr\" that writes messages to the console\n"
                    "[log.console_appender.stderr]\n"
                    "stream=std_error\n\n"
                    "# declare an appender named \"p2p\" that writes messages to p2p.log\n"
                    "[log.file_appender.p2p]\n"
                    "filename=logs/p2p/p2p.log\n"
                    "# filename can be absolute or relative to this config file\n\n"
                    "# route any messages logged to the default logger to the \"stderr\" logger we\n"
                    "# declared above, if they are info level are higher\n"
                    "[logger.default]\n"
                    "level=warn\n"
                    "appenders=stderr\n\n"
                    "# route messages sent to the \"p2p\" logger to the p2p appender declared above\n"
                    "[logger.p2p]\n"
                    "level=warn\n"
                    "appenders=p2p\n\n";
}

fc::optional<fc::logging_config> load_logging_config_from_ini_file(const fc::path &config_ini_filename) {
    try {
        fc::logging_config logging_config;
        bool found_logging_config = false;

        boost::property_tree::ptree config_ini_tree;
        boost::property_tree::ini_parser::read_ini(config_ini_filename.preferred_string().c_str(), config_ini_tree);
        for (const auto &section : config_ini_tree) {
            const std::string &section_name = section.first;
            const boost::property_tree::ptree &section_tree = section.second;

            const std::string console_appender_section_prefix = "log.console_appender.";
            const std::string file_appender_section_prefix = "log.file_appender.";
            const std::string logger_section_prefix = "logger.";

            if (boost::starts_with(section_name, console_appender_section_prefix)) {
                std::string console_appender_name = section_name.substr(console_appender_section_prefix.length());
                std::string stream_name = section_tree.get<std::string>("stream");

                // construct a default console appender config here
                // stdout/stderr will be taken from ini file, everything else hard-coded here
                fc::console_appender::config console_appender_config;
                console_appender_config.level_colors.emplace_back(
                        fc::console_appender::level_color(fc::log_level::debug,
                                fc::console_appender::color::green));
                console_appender_config.level_colors.emplace_back(
                        fc::console_appender::level_color(fc::log_level::warn,
                                fc::console_appender::color::brown));
                console_appender_config.level_colors.emplace_back(
                        fc::console_appender::level_color(fc::log_level::error,
                                fc::console_appender::color::cyan));
                console_appender_config.stream = fc::variant(stream_name).as<fc::console_appender::stream::type>();
                logging_config.appenders.push_back(fc::appender_config(console_appender_name, "console", fc::variant(console_appender_config)));
                found_logging_config = true;
            } else if (boost::starts_with(section_name, file_appender_section_prefix)) {
                std::string file_appender_name = section_name.substr(file_appender_section_prefix.length());
                fc::path file_name = section_tree.get<std::string>("filename");
                if (file_name.is_relative()) {
                    file_name =
                            fc::absolute(config_ini_filename).parent_path() /
                            file_name;
                }


                // construct a default file appender config here
                // filename will be taken from ini file, everything else hard-coded here
                fc::file_appender::config file_appender_config;
                file_appender_config.filename = file_name;
                file_appender_config.flush = true;
                file_appender_config.rotate = true;
                file_appender_config.rotation_interval = fc::hours(1);
                file_appender_config.rotation_limit = fc::days(1);
                logging_config.appenders.push_back(fc::appender_config(file_appender_name, "file", fc::variant(file_appender_config)));
                found_logging_config = true;
            } else if (boost::starts_with(section_name, logger_section_prefix)) {
                std::string logger_name = section_name.substr(logger_section_prefix.length());
                std::string level_string = section_tree.get<std::string>("level");
                std::string appenders_string = section_tree.get<std::string>("appenders");
                fc::logger_config logger_config(logger_name);
                logger_config.level = fc::variant(level_string).as<fc::log_level>();
                boost::split(logger_config.appenders, appenders_string,
                        boost::is_any_of(" ,"),
                        boost::token_compress_on);
                logging_config.loggers.push_back(logger_config);
                found_logging_config = true;
            }
        }
        if (found_logging_config) {
            return logging_config;
        } else {
            return fc::optional<fc::logging_config>();
        }
    }
    FC_RETHROW_EXCEPTIONS(warn, "")
}
 */

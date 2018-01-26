#include <appbase/application.hpp>
#include <golos/protocol/types.hpp>
#include <fc/log/logger_config.hpp>
#include <graphene/utilities/key_conversion.hpp>
#include <fc/git_revision.hpp>
#include <golos/plugins/chain/plugin.hpp>
#include <golos/plugins/p2p/p2p_plugin.hpp>
#include <golos/plugins/webserver/webserver_plugin.hpp>
#include <golos/plugins/network_broadcast_api/network_broadcast_api_plugin.hpp>
#include <golos/plugins/witness/witness.hpp>
#include <golos/plugins/database_api/plugin.hpp>
#include <golos/plugins/market_history/market_history_plugin.hpp>
#include <golos/plugins/test_api/test_api_plugin.hpp>
#include <golos/plugins/social_network/social_network.hpp>
#include <golos/plugins/account_history/plugin.hpp>
#include <golos/plugins/blockchain_statistics/plugin.hpp>
#include <golos/plugins/account_by_key/account_by_key_plugin.hpp>
#include <golos/plugins/auth_util/plugin.hpp>

#include <fc/interprocess/signals.hpp>
#include <fc/log/console_appender.hpp>
#include <fc/log/json_console_appender.hpp>
#include <fc/log/file_appender.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <graphene/utilities/git_revision.hpp>

using golos::protocol::version;


std::string& version_string() {
    static std::string v_str =
            "golos_blockchain_version: " + std::string( STEEMIT_BLOCKCHAIN_VERSION ) + "\n" +
            "golos_git_revision:       " + std::string( golos::utilities::git_revision_sha )       + "\n" +
            "fc_git_revision:          " + std::string( fc::git_revision_sha )       + "\n";
    return v_str;
}

namespace golos {

    namespace utilities {
        void set_logging_program_options(boost::program_options::options_description &);
        fc::optional<fc::logging_config> load_logging_config( const boost::program_options::variables_map&, const boost::filesystem::path&);
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
///plugins
        appbase::app().register_plugin<golos::plugins::chain::plugin>();
        appbase::app().register_plugin<golos::plugins::p2p::p2p_plugin>();
        appbase::app().register_plugin<golos::plugins::webserver::webserver_plugin>();
        appbase::app().register_plugin<golos::plugins::witness_plugin::witness_plugin>();
        appbase::app().register_plugin<golos::plugins::network_broadcast_api::network_broadcast_api_plugin>();
        golos::plugins::database_api::register_database_api();
        appbase::app().register_plugin<golos::plugins::social_network::social_network_t>();
        appbase::app().register_plugin<golos::plugins::test_api::test_api_plugin>();
        appbase::app().register_plugin<golos::plugins::market_history::market_history_plugin>();
        appbase::app().register_plugin<golos::plugins::account_history::plugin>();
        appbase::app().register_plugin<golos::plugins::blockchain_statistics::plugin>();
        appbase::app().register_plugin<golos::plugins::account_by_key::account_by_key_plugin>();
        appbase::app().register_plugin<golos::plugins::auth_util::plugin>();
///plugins
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
            fc::optional< fc::logging_config > logging_config = golos::utilities::load_logging_config( args, appbase::app().data_dir() / "config.ini" );
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
namespace utilities {
using std::string;
using std::vector;

void set_logging_program_options(boost::program_options::options_description &options) {
    /*
    out << "# declare an appender named \"stderr\" that writes messages to the console\n"
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
            */
}


fc::optional<fc::logging_config> load_logging_config(const boost::program_options::variables_map &args, const boost::filesystem::path &pwd) {
    try {
        fc::logging_config logging_config;
        bool found_logging_config = false;

        boost::property_tree::ptree config_ini_tree;
        boost::property_tree::ini_parser::read_ini(pwd.string(), config_ini_tree);
        for (const auto &section : config_ini_tree) {            
            const std::string &section_name = section.first;
            const boost::property_tree::ptree &section_tree = section.second;

            const std::string console_appender_section_prefix = "log.console_appender.";
            const std::string json_console_appender_section_prefix = "log.json_console_appender.";
            const std::string file_appender_section_prefix = "log.file_appender.";
            const std::string logger_section_prefix = "logger.";

            if (boost::starts_with(section_name, json_console_appender_section_prefix)) {
                std::string console_appender_name = section_name.substr(json_console_appender_section_prefix.length());
                std::string stream_name = section_tree.get<std::string>("stream");

                // construct a default json console appender config here
                // stdout/stderr will be taken from ini file, everything else hard-coded here
                fc::json_console_appender::j_config console_appender_config;
                console_appender_config.level_colors.emplace_back(fc::json_console_appender::j_level_color(fc::log_level::debug, fc::json_console_appender::j_color::green));
                console_appender_config.level_colors.emplace_back(fc::json_console_appender::j_level_color(fc::log_level::warn, fc::json_console_appender::j_color::brown));
                console_appender_config.level_colors.emplace_back(fc::json_console_appender::j_level_color(fc::log_level::error, fc::json_console_appender::j_color::cyan));
                console_appender_config.stream = fc::variant(stream_name).as<fc::json_console_appender::j_stream::type>();
                logging_config.appenders.emplace_back(console_appender_name, "json_console", fc::variant(console_appender_config));
                found_logging_config = true;
            } else if (boost::starts_with(section_name, console_appender_section_prefix)) {
                std::string console_appender_name = section_name.substr(console_appender_section_prefix.length());
                std::string stream_name = section_tree.get<std::string>("stream");

                // construct a default console appender config here
                // stdout/stderr will be taken from ini file, everything else hard-coded here
                fc::console_appender::config console_appender_config;
                console_appender_config.level_colors.emplace_back(fc::console_appender::level_color(fc::log_level::debug, fc::console_appender::color::green));
                console_appender_config.level_colors.emplace_back(fc::console_appender::level_color(fc::log_level::warn, fc::console_appender::color::brown));
                console_appender_config.level_colors.emplace_back(fc::console_appender::level_color(fc::log_level::error, fc::console_appender::color::cyan));
                console_appender_config.stream = fc::variant(stream_name).as<fc::console_appender::stream::type>();
                logging_config.appenders.emplace_back(console_appender_name, "console", fc::variant(console_appender_config));
                found_logging_config = true;
            } else if (boost::starts_with(section_name, file_appender_section_prefix)) {
                std::string file_appender_name = section_name.substr(file_appender_section_prefix.length());
                fc::path file_name = section_tree.get<std::string>("filename");
                if (file_name.is_relative()) {
                    file_name = fc::absolute(pwd).parent_path() / file_name;
                }


                // construct a default file appender config here
                // filename will be taken from ini file, everything else hard-coded here
                fc::file_appender::config file_appender_config;
                file_appender_config.filename = file_name;
                file_appender_config.flush = true;
                file_appender_config.rotate = true;
                file_appender_config.rotation_interval = fc::hours(1);
                file_appender_config.rotation_limit = fc::days(1);
                logging_config.appenders.emplace_back(file_appender_name, "file", fc::variant(file_appender_config));
                found_logging_config = true;
            } else if (boost::starts_with(section_name, logger_section_prefix)) {
                std::string logger_name = section_name.substr(logger_section_prefix.length());
                std::string level_string = section_tree.get<std::string>("level");
                std::string appenders_string = section_tree.get<std::string>("appenders");
                fc::logger_config logger_config(logger_name);
                logger_config.level = fc::variant(level_string).as<fc::log_level>();
                boost::split(logger_config.appenders, appenders_string, boost::is_any_of(" ,"), boost::token_compress_on);
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
}
}

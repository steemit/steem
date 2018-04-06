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
#include <golos/plugins/private_message/private_message_plugin.hpp>
#include <golos/plugins/auth_util/plugin.hpp>
#include <golos/plugins/debug_node/plugin.hpp>
#include <golos/plugins/raw_block/plugin.hpp>
#include <golos/plugins/block_info/plugin.hpp>

#include <fc/interprocess/signals.hpp>
#include <fc/log/console_appender.hpp>
#include <fc/log/json_console_appender.hpp>
#include <fc/log/file_appender.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string.hpp>

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
        namespace bpo = boost::program_options;
        void set_logging_program_options(bpo::options_description &);
        fc::optional<fc::logging_config> load_logging_config(const boost::filesystem::path&);
        std::map<string, std::map<string, string>> load_config_sections(const boost::filesystem::path &path);
    }

    namespace plugins {
        void register_plugins(){
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
            appbase::app().register_plugin<golos::plugins::private_message::private_message_plugin>();
            appbase::app().register_plugin<golos::plugins::auth_util::plugin>();
            appbase::app().register_plugin<golos::plugins::raw_block::plugin>();
            appbase::app().register_plugin<golos::plugins::block_info::plugin>();
            appbase::app().register_plugin<golos::plugins::debug_node::plugin>();
            ///plugins
        };
    }

}

void logo(){

#ifdef STEEMIT_BUILD_TESTNET
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

        golos::plugins::register_plugins();
        appbase::app().set_version_string(version_string());

        // Setup logging config
        boost::program_options::options_description options;
        golos::utilities::set_logging_program_options(options);
        appbase::app().add_program_options(boost::program_options::options_description(), options);

        bool initialized = appbase::app().initialize<
                golos::plugins::chain::plugin,
                golos::plugins::p2p::p2p_plugin,
                golos::plugins::webserver::webserver_plugin
        >
                ( argc, argv );

        if( !initialized )
            return 0;

        logo();

        // auto& args = appbase::app().get_args();

        try {
            fc::optional<fc::logging_config> logging_config = golos::utilities::load_logging_config(appbase::app().config_path());
            if (logging_config)
                fc::configure_logging(*logging_config);
        } catch (const fc::exception&) {
            wlog("Error parsing logging config");
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

void set_logging_program_options(bpo::options_description &options) {
    bpo::options_description log_opts("Logging options");
    log_opts.add_options()
        // pass section description with trailing dot (declare before section options)
        ("log.console_appender.stderr.", "declare an appender named \"stderr\" that writes messages to the console")
        ("log.console_appender.stderr.stream", bpo::value<string>()->default_value("std_error"), "console appender stream")
        ("log.file_appender.p2p.", "declare an appender named \"p2p\" that writes messages to p2p.log")
        ("log.file_appender.p2p.filename", bpo::value<string>()->default_value("logs/p2p/p2p.log"), "filename can be absolute or relative to this config file")
        // The following 4 options can be implemented in load_logging_config()
        // ("log.file_appender.p2p.flush", bpo::value<bool>()->default_value(true), "use flush (true|false|yes|no|1|0)")
        // ("log.file_appender.p2p.rotate", bpo::value<bool>()->default_value(true), "use rotate (true|false|yes|no|1|0)")
        // ("log.file_appender.p2p.rotation_interval", bpo::value<int64_t>()->default_value(fc::hours(1).to_seconds()), "rotation interval in seconds")
        // ("log.file_appender.p2p.rotation_limit", bpo::value<int64_t>()->default_value(fc::days(1).to_seconds()), "rotation limit in seconds")

        ("logger.default.", "route any messages logged to the default logger to the \"stderr\" logger we declared above, if they are info level are higher")
        ("logger.default.level", bpo::value<string>()->default_value("warn"), "log level of \"default\" logger (all|debug|info|warn|error|off)")
        ("logger.default.appenders", bpo::value<string>()->default_value("stderr"), "appender(s) of \"default\" logger")

        ("logger.p2p.", "route messages sent to the \"p2p\" logger to the p2p appender declared above")
        ("logger.p2p.level", bpo::value<string>()->default_value("warn"), "log level of \"p2p\" logger (all|debug|info|warn|error|off)")
        ("logger.p2p.appenders", bpo::value<string>()->default_value("p2p"), "appender(s) of \"p2p\" logger")
        ;

    options.add(log_opts);
}


std::map<string, std::map<string, string>> load_config_sections(const boost::filesystem::path &path) {
    bpo::options_description options;
    auto parsed_options = bpo::parse_config_file<char>(path.string().c_str(), options, true);
    std::map<string, std::map<string, string>> sections;
    for (const auto& o : parsed_options.options) {
        auto key = o.string_key;
        auto pos = key.rfind('.');
        if (pos != std::string::npos) {
            auto s = key.substr(0, pos);
            auto k = key.substr(pos+1);
            auto v = o.value;
            sections[s][k] = v.empty() ? "" : v[0];
        }
    }
    return sections;
}

fc::optional<fc::logging_config> load_logging_config(const boost::filesystem::path &pwd) {
    try {
        fc::logging_config logging_config;
        bool found_logging_config = false;

        const std::string console_appender_section_prefix = "log.console_appender.";
        const std::string json_console_appender_section_prefix = "log.json_console_appender.";
        const std::string file_appender_section_prefix = "log.file_appender.";
        const std::string logger_section_prefix = "logger.";

        auto sections = load_config_sections(pwd);
        for (const auto& sect : sections) {
            const std::string &section_name = sect.first;
            auto &options = sect.second;

            if (boost::starts_with(section_name, json_console_appender_section_prefix)) {
                std::string console_appender_name = section_name.substr(json_console_appender_section_prefix.length());
                std::string stream_name = options.at("stream");

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
                std::string stream_name = options.at("stream");

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
                fc::path file_name = options.at("filename");
                if (file_name.is_relative()) {
                    file_name = fc::absolute(pwd).parent_path() / file_name;
                }

                // construct a default file appender config here
                // filename will be taken from ini file, everything else hard-coded here
                fc::file_appender::config file_appender_config;
                file_appender_config.filename = file_name;
                // NOTE: the following 4 options can be loaded too, sample code in cli_wallet
                file_appender_config.flush = true;
                file_appender_config.rotate = true;
                file_appender_config.rotation_interval = fc::hours(1);
                file_appender_config.rotation_limit = fc::days(1);
                logging_config.appenders.emplace_back(file_appender_name, "file", fc::variant(file_appender_config));
                found_logging_config = true;
            } else if (boost::starts_with(section_name, logger_section_prefix)) {
                std::string logger_name = section_name.substr(logger_section_prefix.length());
                std::string level_string = options.at("level");
                std::string appenders_string = options.at("appenders");
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

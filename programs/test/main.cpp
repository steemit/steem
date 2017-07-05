#include <appbase/application.hpp>
#include <steemit/plugins/http/http_plugin.hpp>
#include <steemit/plugins/test_api/test_api_plugin.hpp>
#include <steemit/plugins/api_register/api_register_plugin.hpp>
#include <steemit/plugins/chain/chain_plugin.hpp>
#include <steemit/plugins/p2p/p2p_plugin.hpp>

#include <fc/exception/exception.hpp>
#include <fc/thread/thread.hpp>
#include <fc/interprocess/signals.hpp>
#include <fc/log/console_appender.hpp>
#include <fc/log/file_appender.hpp>
#include <fc/log/logger.hpp>
#include <fc/log/logger_config.hpp>

#include <boost/exception/diagnostic_information.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <iostream>
#include <csignal>

using steemit::protocol::version;

fc::optional<fc::logging_config> load_logging_config_from_ini_file();

int main( int argc, char** argv )
{
   try
   {
      #ifdef IS_TEST_NET
      std::cerr << "------------------------------------------------------\n\n";
      std::cerr << "            STARTING TEST NETWORK\n\n";
      std::cerr << "------------------------------------------------------\n";
      auto initminer_private_key = graphene::utilities::key_to_wif( STEEMIT_INIT_PRIVATE_KEY );
      std::cerr << "initminer public key: " << STEEMIT_INIT_PUBLIC_KEY_STR << "\n";
      std::cerr << "initminer private key: " << initminer_private_key << "\n";
      std::cerr << "chain id: " << std::string(STEEMIT_CHAIN_ID) << "\n";
      std::cerr << "blockchain version: " << fc::string( STEEMIT_BLOCKCHAIN_VERSION ) << "\n";
      std::cerr << "------------------------------------------------------\n";
#else
      std::cerr << "------------------------------------------------------\n\n";
      std::cerr << "            STARTING STEEM NETWORK\n\n";
      std::cerr << "------------------------------------------------------\n";
      std::cerr << "initminer public key: " << STEEMIT_INIT_PUBLIC_KEY_STR << "\n";
      std::cerr << "chain id: " << std::string(STEEMIT_CHAIN_ID) << "\n";
      std::cerr << "blockchain version: " << fc::string( STEEMIT_BLOCKCHAIN_VERSION ) << "\n";
      std::cerr << "------------------------------------------------------\n";
#endif

      try
      {
         /*
         fc::optional<fc::logging_config> logging_config = load_logging_config_from_ini_file();
         if (logging_config)
            fc::configure_logging(*logging_config);
         //*/
      }
      catch (const fc::exception&)
      {
         wlog("Error parsing logging config from config");
      }

      appbase::app().register_plugin< steemit::plugins::http::http_plugin >();
      appbase::app().register_plugin< steemit::plugins::api_register::api_register_plugin >();
      appbase::app().register_plugin< steemit::plugins::test_api::test_api_plugin >();
      appbase::app().register_plugin< steemit::plugins::chain::chain_plugin >();
      appbase::app().register_plugin< steemit::plugins::p2p::p2p_plugin >();
      if( !appbase::app().initialize( argc, argv ) )
         return -1;
      appbase::app().startup();
      appbase::app().exec();
   }
   catch ( const boost::exception& e )
   {
      std::cerr << boost::diagnostic_information(e) << "\n";
   }
   catch ( const std::exception& e )
   {
      std::cerr << e.what() << "\n";
   }
   catch ( ... )
   {
      std::cerr << "unknown exception\n";
   }

   std::cout << "exited cleanly\n";
   return 0;
}

fc::optional<fc::logging_config> load_logging_config_from_ini_file()
{
   try
   {
      fc::logging_config logging_config;
      bool found_logging_config = false;

      //boost::property_tree::ptree config_ini_tree;
      //boost::property_tree::ini_parser::read_ini(config_ini_filename.preferred_string().c_str(), config_ini_tree);
         //if (boost::starts_with(section_name, console_appender_section_prefix))
         {
            //std::string console_appender_name = section_name.substr(console_appender_section_prefix.length());
            //std::string stream_name = section_tree.get<std::string>("stream");

            std::string console_appender_name = "stderr";
            std::string stream_name = "std_error";

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
         }

         //else if (boost::starts_with(section_name, logger_section_prefix))
         {
            //std::string logger_name = section_name.substr(logger_section_prefix.length());
            //std::string level_string = section_tree.get<std::string>("level");
            //std::string appenders_string = section_tree.get<std::string>("appenders");
            std::string logger_name = "default";
            std::string level_string = "debug";
            std::string appenders_string = "stderr";
            fc::logger_config logger_config(logger_name);
            logger_config.level = fc::variant(level_string).as<fc::log_level>();
            boost::split(logger_config.appenders, appenders_string,
                         boost::is_any_of(" ,"),
                         boost::token_compress_on);
            logging_config.loggers.push_back(logger_config);
            found_logging_config = true;
         }
         {
            std::string logger_name = "p2p";
            std::string level_string = "debug";
            std::string appenders_string = "stderr";
            fc::logger_config logger_config(logger_name);
            logger_config.level = fc::variant(level_string).as<fc::log_level>();
            boost::split(logger_config.appenders, appenders_string,
                         boost::is_any_of(" ,"),
                         boost::token_compress_on);
            logging_config.loggers.push_back(logger_config);
            found_logging_config = true;
         }
      if (found_logging_config)
         return logging_config;
      else
         return fc::optional<fc::logging_config>();
   }
   FC_RETHROW_EXCEPTIONS(warn, "")
}
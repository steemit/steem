#include <appbase/application.hpp>
#include <steemit/plugins/webserver/webserver_plugin.hpp>
#include <steemit/plugins/test_api/test_api_plugin.hpp>
#include <steemit/plugins/json_rpc/json_rpc_plugin.hpp>
#include <steemit/plugins/chain/chain_plugin.hpp>
#include <steemit/plugins/p2p/p2p_plugin.hpp>
#include <steemit/plugins/database_api/database_api_plugin.hpp>
#include <steemit/plugins/network_broadcast_api/network_broadcast_api_plugin.hpp>
#include <steemit/plugins/witness/witness_plugin.hpp>
#include <steemit/plugins/account_by_key/account_by_key_plugin.hpp>
#include <steemit/plugins/account_by_key_api/account_by_key_api_plugin.hpp>
#include <steemit/plugins/account_history/account_history_plugin.hpp>

#include <fc/exception/exception.hpp>
#include <fc/thread/thread.hpp>
#include <fc/interprocess/signals.hpp>
#include <fc/log/console_appender.hpp>
#include <fc/log/file_appender.hpp>
#include <fc/log/logger.hpp>
#include <fc/log/logger_config.hpp>

#include <boost/exception/diagnostic_information.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <csignal>
#include <vector>

namespace bpo = boost::program_options;
using steemit::protocol::version;
using std::string;
using std::vector;

fc::optional<fc::logging_config> load_logging_config( const bpo::variables_map& );

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
      std::cerr << "chain id: " << std::string( STEEMIT_CHAIN_ID ) << "\n";
      std::cerr << "blockchain version: " << fc::string( STEEMIT_BLOCKCHAIN_VERSION ) << "\n";
      std::cerr << "------------------------------------------------------\n";
#else
      std::cerr << "------------------------------------------------------\n\n";
      std::cerr << "            STARTING STEEM NETWORK\n\n";
      std::cerr << "------------------------------------------------------\n";
      std::cerr << "initminer public key: " << STEEMIT_INIT_PUBLIC_KEY_STR << "\n";
      std::cerr << "chain id: " << std::string( STEEMIT_CHAIN_ID ) << "\n";
      std::cerr << "blockchain version: " << fc::string( STEEMIT_BLOCKCHAIN_VERSION ) << "\n";
      std::cerr << "------------------------------------------------------\n";
#endif

      // Setup logging config
      bpo::options_description options;

      std::vector< std::string > default_console_appender( {"stderr","std_err"} );
      std::string str_default_console_appender = boost::algorithm::join( default_console_appender, " " );

      std::vector< std::string > default_file_appender( {"p2p","logs/p2p/p2p.log"} );
      std::string str_default_file_appender = boost::algorithm::join( default_file_appender, " " );

      std::vector< std::string > default_logger( {"default","warn","stderr","p2p","warn","p2p"} );
      std::string str_default_logger = boost::algorithm::join( default_logger, " " );

      options.add_options()
         ("log-console-appender", bpo::value< std::vector< std::string > >()->composing()->default_value( default_console_appender, str_default_console_appender ),
            "Console appender definitions: as name stream" )
         ("log-file-appender", bpo::value< std::vector< std::string > >()->composing()->default_value( default_file_appender, str_default_file_appender ),
            "File appender definitions: as name file" )
         ("log-logger", bpo::value< std::vector< std::string > >()->composing()->default_value( default_logger, str_default_logger ),
            "Logger definition as: name level appender" )
         ;

      appbase::app().add_program_options( bpo::options_description(), options );

      appbase::app().register_plugin< steemit::plugins::webserver::webserver_plugin >();
      appbase::app().register_plugin< steemit::plugins::json_rpc::json_rpc_plugin >();
      appbase::app().register_plugin< steemit::plugins::test_api::test_api_plugin >();
      appbase::app().register_plugin< steemit::plugins::chain::chain_plugin >();
      appbase::app().register_plugin< steemit::plugins::p2p::p2p_plugin >();
      appbase::app().register_plugin< steemit::plugins::database_api::database_api_plugin >();
      appbase::app().register_plugin< steemit::plugins::network_broadcast_api::network_broadcast_api_plugin >();
      appbase::app().register_plugin< steemit::plugins::witness::witness_plugin >();
      appbase::app().register_plugin< steemit::plugins::account_by_key::account_by_key_plugin >();
      appbase::app().register_plugin< steemit::plugins::account_by_key_api::account_by_key_api_plugin >();
      appbase::app().register_plugin< steemit::plugins::account_history::account_history_plugin >();

      if( !appbase::app().initialize( argc, argv ) )
         return -1;

      try
      {
         fc::optional< fc::logging_config > logging_config = load_logging_config( appbase::app().get_args() );
         if( logging_config )
            fc::configure_logging( *logging_config );
      }
      catch( const fc::exception& )
      {
         wlog( "Error parsing logging config" );
      }

      appbase::app().startup();
      appbase::app().exec();
      std::cout << "exited cleanly\n";
      return 0;
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

   return -1;
}

vector< string > tokenize_config_args( const vector< string >& args )
{
   vector< string > result;

   for( auto& a : args )
   {
      vector< string > tokens;
      boost::split( tokens, a, boost::is_any_of( " \t" ) );
      for( const auto& t : tokens )
         if( t.size() )
            result.push_back( t );
   }

   return result;
}

fc::optional<fc::logging_config> load_logging_config( const bpo::variables_map& args )
{
   try
   {
      fc::logging_config logging_config;
      bool found_logging_config = false;

      if( args.count( "log-console-appender" ) )
      {
         std::vector< string > console_appender_args = tokenize_config_args( args["log-console-appender"].as< vector< string > >() );
         uint32_t root = 0;

         while( console_appender_args.size() - root >= 2 )
         {
            std::string console_appender_name = console_appender_args[ root ];
            std::string stream_name = console_appender_args[ root + 1 ];

            // construct a default console appender config here
            // stdout/stderr will be taken from ini file, everything else hard-coded here
            fc::console_appender::config console_appender_config;
            console_appender_config.level_colors.emplace_back(
               fc::console_appender::level_color( fc::log_level::debug,
                                                  fc::console_appender::color::green));
            console_appender_config.level_colors.emplace_back(
               fc::console_appender::level_color( fc::log_level::warn,
                                                  fc::console_appender::color::brown));
            console_appender_config.level_colors.emplace_back(
               fc::console_appender::level_color( fc::log_level::error,
                                                  fc::console_appender::color::red));
            console_appender_config.stream = fc::variant( stream_name ).as< fc::console_appender::stream::type >();
            logging_config.appenders.push_back(
               fc::appender_config( console_appender_name, "console", fc::variant( console_appender_config ) ) );
            found_logging_config = true;
            root += 2;
         }
      }

      if( args.count( "log-file-appender" ) )
      {
         std::vector< string > file_appender_args = tokenize_config_args( args["log-file-appender"].as< vector< string > >() );
         uint32_t root = 0;

         while( file_appender_args.size() - root >= 2 )
         {
            std::string file_appender_name = file_appender_args[ root ];
            fc::path file_name = file_appender_args[ root + 1 ];
            if( file_name.is_relative() )
               file_name = fc::absolute( appbase::app().data_dir() ) / file_name;

            // construct a default file appender config here
            // filename will be taken from ini file, everything else hard-coded here
            fc::file_appender::config file_appender_config;
            file_appender_config.filename = file_name;
            file_appender_config.flush = true;
            file_appender_config.rotate = true;
            file_appender_config.rotation_interval = fc::hours(1);
            file_appender_config.rotation_limit = fc::days(1);
            logging_config.appenders.push_back(
               fc::appender_config( file_appender_name, "file", fc::variant( file_appender_config ) ) );
            found_logging_config = true;
            root += 2;
         }
      }

      if( args.count( "log-logger" ) )
      {
         std::vector< string > logger_args = tokenize_config_args( args[ "log-logger" ].as< std::vector< std::string > >() );
         uint32_t root = 0;

         while( logger_args.size() - root >= 3 )
         {
            std::string logger_name = logger_args[ root ];
            std::string level_string = logger_args[ root + 1 ];
            std::string appenders_string = logger_args[ root + 2 ];
            fc::logger_config logger_config( logger_name );
            logger_config.level = fc::variant( level_string ).as< fc::log_level >();
            boost::split( logger_config.appenders, appenders_string,
                          boost::is_any_of(" ,"),
                          boost::token_compress_on );
            logging_config.loggers.push_back( logger_config );
            found_logging_config = true;
            root += 3;
         }
      }

      if( found_logging_config )
         return logging_config;
      else
         return fc::optional< fc::logging_config >();
   }
   FC_RETHROW_EXCEPTIONS(warn, "")
}
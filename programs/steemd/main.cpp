#include <appbase/application.hpp>
#include <steemit/manifest/plugins.hpp>

#include <steemit/protocol/types.hpp>
#include <steemit/protocol/version.hpp>

#include <steemit/utilities/logging_config.hpp>
#include <steemit/utilities/key_conversion.hpp>
#include <steemit/utilities/git_revision.hpp>

#include <steemit/plugins/chain/chain_plugin.hpp>
#include <steemit/plugins/p2p/p2p_plugin.hpp>
#include <steemit/plugins/webserver/webserver_plugin.hpp>

#include <fc/exception/exception.hpp>
#include <fc/thread/thread.hpp>
#include <fc/interprocess/signals.hpp>
#include <fc/git_revision.hpp>

#include <boost/exception/diagnostic_information.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <csignal>
#include <vector>

namespace bpo = boost::program_options;
using steemit::protocol::version;
using std::string;
using std::vector;

string& version_string()
{
   static string v_str =
      "steem_blockchain_version: " + fc::string( STEEMIT_BLOCKCHAIN_VERSION ) + "\n" +
      "steem_git_revision:       " + fc::string( steemit::utilities::git_revision_sha ) + "\n" +
      "fc_git_revision:          " + fc::string( fc::git_revision_sha ) + "\n";
   return v_str;
}

int main( int argc, char** argv )
{
   try
   {
#ifdef IS_TEST_NET
      std::cerr << "------------------------------------------------------\n\n";
      std::cerr << "            STARTING TEST NETWORK\n\n";
      std::cerr << "------------------------------------------------------\n";
      auto initminer_private_key = steemit::utilities::key_to_wif( STEEMIT_INIT_PRIVATE_KEY );
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

      steemit::utilities::set_logging_program_options( options );
      appbase::app().add_program_options( bpo::options_description(), options );

      steemit::plugins::register_plugins();
      appbase::app().set_version_string( version_string() );

      if( !appbase::app().initialize<
            steemit::plugins::chain::chain_plugin,
            steemit::plugins::p2p::p2p_plugin,
            steemit::plugins::webserver::webserver_plugin >
            ( argc, argv ) )
         return 0;

      try
      {
         fc::optional< fc::logging_config > logging_config = steemit::utilities::load_logging_config( appbase::app().get_args(), appbase::app().data_dir() );
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

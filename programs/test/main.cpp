#include <appbase/application.hpp>
#include <steemit/plugins/http/http_plugin.hpp>
#include <steemit/plugins/test_api/test_api_plugin.hpp>
#include <steemit/plugins/api_register/api_register_plugin.hpp>

#include <fc/exception/exception.hpp>
#include <fc/thread/thread.hpp>
#include <fc/interprocess/signals.hpp>
#include <fc/log/logger.hpp>
#include <fc/log/logger_config.hpp>

#include <boost/exception/diagnostic_information.hpp>

#include <iostream>
#include <csignal>

int main( int argc, char** argv )
{
   try
   {
      appbase::app().register_plugin< steemit::plugins::http::http_plugin >();
      appbase::app().register_plugin< steemit::plugins::api_register::api_register_plugin >();
      appbase::app().register_plugin< steemit::plugins::test_api::test_api_plugin >();
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
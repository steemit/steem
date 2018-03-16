#include <fc/network/udt_socket.hpp>
#include <fc/network/ip.hpp>
#include <fc/exception/exception.hpp>
#include <fc/thread/thread.hpp>
#include <iostream>
#include <vector>

using namespace fc;

int main( int argc, char** argv )
{
   try {
       udt_socket sock;
       sock.bind( fc::ip::endpoint::from_string( "127.0.0.1:6666" ) );
       ilog( "." );
       sock.connect_to( fc::ip::endpoint::from_string( "127.0.0.1:7777" ) );
       ilog( "after connect to..." );

       std::cout << "local endpoint: " <<std::string( sock.local_endpoint() ) <<"\n";
       std::cout << "remote endpoint: " <<std::string( sock.remote_endpoint() ) <<"\n";

       std::string hello = "hello world\n";
       for( uint32_t i = 0; i < 1000000; ++i )
       {
          sock.write( hello.c_str(), hello.size() );
       }
       ilog( "closing" );
       sock.close();
       usleep( fc::seconds(1) );
       /*
       std::vector<char> response;
       response.resize(1024);
       int r = sock.readsome( response.data(), response.size() );
       while( r )
       {
          std::cout.write( response.data(), r );
          r = sock.readsome( response.data(), response.size() );
       }
       */
       // if we exit too quickly, UDT will not have a chance to
       // send the graceful close message.
       //fc::usleep( fc::seconds(1) );
   } catch ( const fc::exception& e )
   {
      elog( "${e}", ("e",e.to_detail_string() ) );
   }

    return 0;
}

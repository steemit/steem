#include <fc/network/udt_socket.hpp>
#include <fc/network/ip.hpp>
#include <fc/exception/exception.hpp>
#include <iostream>
#include <vector>

using namespace fc;

int main( int argc, char** argv )
{
   try {
      udt_server serv;
      serv.listen( fc::ip::endpoint::from_string( "127.0.0.1:7777" ) );

      while( true )
      {
          udt_socket sock;
          serv.accept( sock );

          std::vector<char> response;
          response.resize(1024);
          int r = sock.readsome( response.data(), response.size() );
          while( r )
          {
             std::cout.write( response.data(), r );
             r = sock.readsome( response.data(), response.size() );
             //sock.write( response.data(), response.size() );
          }

          std::string goodbye = "goodbye cruel world";
          sock.write( goodbye.c_str(), goodbye.size() );
      }
   } catch ( const fc::exception& e )
   {
      elog( "${e}", ("e",e.to_detail_string() ) );
   }

   return 0;
}

#include <arpa/inet.h>
#include <udt.h>
#include <iostream>

using namespace std;

int main( int argc, char** argv )
{
   UDTSOCKET serv = UDT::socket(AF_INET, SOCK_STREAM, 0);
   bool block = false;

   sockaddr_in my_addr;
   my_addr.sin_family = AF_INET;
   my_addr.sin_port = htons(9000);
   my_addr.sin_addr.s_addr = INADDR_ANY;
   memset(&(my_addr.sin_zero), '\0', 8);

   if (UDT::ERROR == UDT::bind(serv, (sockaddr*)&my_addr, sizeof(my_addr)))
   {
     cout << "bind: " << UDT::getlasterror().getErrorMessage();
     return 0;
   }
   UDT::listen(serv, 10);

   int namelen;
   sockaddr_in their_addr;


   UDT::setsockopt(serv, 0, UDT_SNDSYN, &block, sizeof(bool));
   UDT::setsockopt(serv, 0, UDT_RCVSYN, &block, sizeof(bool));
   UDTSOCKET recver = UDT::accept(serv, (sockaddr*)&their_addr, &namelen);
   if( recver == UDT::INVALID_SOCK )
   {
      if( UDT::getlasterror_code() == CUDTException::EASYNCRCV )
      {
         std::cout << "nothing yet... better luck next time\n";
      }
   }
   auto pollid = UDT::epoll_create();
   UDT::epoll_add_usock(pollid, serv, nullptr );// const int* events = NULL);
   std::set<UDTSOCKET> readready;
   std::set<UDTSOCKET> writeready;
   std::cout << "waiting for 5 seconds\n";
   UDT::epoll_wait( pollid, &readready, &writeready, 10000 );
   

   recver = UDT::accept(serv, (sockaddr*)&their_addr, &namelen);
   if( recver == UDT::INVALID_SOCK )
   {
      if( UDT::getlasterror_code() == CUDTException::EASYNCRCV )
      {
         std::cout << "nothing yet... better luck next time\n";
      }
      return 0;
   }
   UDT::setsockopt(recver, 0, UDT_SNDSYN, &block, sizeof(bool));
   UDT::setsockopt(recver, 0, UDT_RCVSYN, &block, sizeof(bool));
   UDT::epoll_remove_usock(pollid, serv );// const int* events = NULL);
   int events = UDT_EPOLL_IN;

   UDT::epoll_add_usock(pollid, recver, &events );// const int* events = NULL);

   readready.clear();
   UDT::epoll_wait( pollid, &readready, &writeready, 5000 );

   char ip[16];
   cout << "new connection: " << inet_ntoa(their_addr.sin_addr) << ":" << ntohs(their_addr.sin_port) << endl;

   char data[100];

   while (UDT::ERROR == UDT::recv(recver, data, 100, 0))
   {
     cout << "recv:" << UDT::getlasterror().getErrorMessage() << endl;
     UDT::epoll_wait( pollid, &readready, &writeready, 5000 );
   }

   cout << data << endl;

   UDT::close(recver);
   UDT::close(serv);

   return 1;
}

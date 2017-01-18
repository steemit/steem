
#include <steemit/time/time.hpp>

#include <fc/optional.hpp>
#include <fc/time.hpp>

#include <fc/exception/exception.hpp>

#include <fc/network/ntp.hpp>

namespace steemit { namespace time {

fc::ntp* ntp_service = nullptr;

void startup_ntp_time()
{
   if( ntp_service == nullptr )
   {
      ntp_service = new fc::ntp;
   }
   else
   {
      wlog( "Called startup_ntp_time() when NTP already running" );
   }
}

void shutdown_ntp_time()
{
   if( ntp_service != nullptr )
   {
      delete ntp_service;
      ntp_service = nullptr;
   }
   else
   {
      wlog( "Called shutdown_ntp_time() when NTP not in use" );
   }
}

void set_ntp_enabled( bool enabled )
{
   bool old_enabled = (ntp_service != nullptr);
   if( (!old_enabled) && enabled )
   {
      ilog( "Calling startup_ntp_time()" );
      startup_ntp_time();
   }
   else if( old_enabled && (!enabled) )
   {
      ilog( "Calling shutdown_ntp_time()" );
      shutdown_ntp_time();
   }
}

fc::time_point now()
{
   if( ntp_service != nullptr )
   {
      fc::optional< fc::time_point > ntp_time = ntp_service->get_time();
      if( ntp_time.valid() )
         return *ntp_time;
   }
   return fc::time_point::now();
}

} } // steemit::time

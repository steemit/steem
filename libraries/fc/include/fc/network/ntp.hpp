#pragma once
#include <string>
#include <memory>
#include <fc/time.hpp>
#include <fc/optional.hpp>


namespace fc {

   namespace detail { class ntp_impl; }

   class ntp 
   {
      public:
         ntp();
         ~ntp();

         void add_server( const std::string& hostname, uint16_t port = 123 );
         void set_request_interval( uint32_t interval_sec );
         void request_now();
         optional<time_point> get_time()const;

      private:
        std::unique_ptr<detail::ntp_impl> my;
   };

} // namespace fc

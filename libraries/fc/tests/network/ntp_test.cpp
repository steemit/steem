#include <boost/test/unit_test.hpp>

#include <fc/network/ntp.hpp>
#include <fc/log/logger.hpp>
#include <fc/thread/thread.hpp>

BOOST_AUTO_TEST_SUITE(fc_network)

BOOST_AUTO_TEST_CASE( ntp_test )
{
   ilog("start ntp test");
   fc::usleep( fc::seconds(1) );
   ilog("done ntp test");
   /*
   fc::ntp ntp_service;
   ntp_service.set_request_interval(5);
   fc::usleep(fc::seconds(4) );
   auto time = ntp_service.get_time();
   BOOST_CHECK( time );
   auto ntp_time = *time;
   auto delta = ntp_time - fc::time_point::now();
//   auto minutes = delta.count() / 1000000 / 60;
//   auto hours = delta.count() / 1000000 / 60 / 60;
//   auto seconds = delta.count() / 1000000;
   auto msec= delta.count() / 1000;
   BOOST_CHECK( msec < 100 );
   */
}

BOOST_AUTO_TEST_SUITE_END()

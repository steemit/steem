#include <fc/real128.hpp>
#include <boost/test/unit_test.hpp>
#include <fc/log/logger.hpp>

BOOST_AUTO_TEST_SUITE(fc)

using fc::real128;
using std::string;

BOOST_AUTO_TEST_CASE(real128_test)
{
   BOOST_CHECK_EQUAL(string(real128()), string("0."));
   BOOST_CHECK_EQUAL(string(real128(0)), string("0."));
   BOOST_CHECK_EQUAL(real128(8).to_uint64(), 8);
   BOOST_CHECK_EQUAL(real128(6789).to_uint64(), 6789);
   BOOST_CHECK_EQUAL(real128(10000).to_uint64(), 10000);
   BOOST_CHECK_EQUAL(string(real128(1)), string("1."));
   BOOST_CHECK_EQUAL(string(real128(5)), string("5."));
   BOOST_CHECK_EQUAL(string(real128(12345)), string("12345."));
   BOOST_CHECK_EQUAL(string(real128(0)), string(real128("0")));

   real128 ten(10);
   real128 two(2);
   real128 twenty(20);
   real128 pi(31415926535);
   pi /=      10000000000;

   BOOST_CHECK_EQUAL( string(ten), "10." );
   BOOST_CHECK_EQUAL( string(two), "2." );
   BOOST_CHECK_EQUAL( string(ten+two), "12." );
   BOOST_CHECK_EQUAL( string(ten-two), "8." );
   BOOST_CHECK_EQUAL( string(ten*two), "20." );
   BOOST_CHECK_EQUAL( string(ten/two), "5." );
   BOOST_CHECK_EQUAL( string(ten/two/two/two*two*two*two), "10." );
   BOOST_CHECK_EQUAL( string(ten/two/two/two*two*two*two), string(ten) );
   BOOST_CHECK_EQUAL( string(twenty/ten), string(two) );
   BOOST_CHECK_EQUAL( string(pi), "3.1415926535" );
   BOOST_CHECK_EQUAL( string(pi*10), "31.415926535" );
   BOOST_CHECK_EQUAL( string(pi*20), "62.83185307" );
   BOOST_CHECK_EQUAL( string(real128("62.83185307")/twenty), string(pi) );
   BOOST_CHECK_EQUAL( string(pi*1), "3.1415926535" );
   BOOST_CHECK_EQUAL( string(pi*0), "0." );

   BOOST_CHECK_EQUAL(real128("12345.6789").to_uint64(), 12345);
   BOOST_CHECK_EQUAL((real128("12345.6789")*10000).to_uint64(), 123456789);
   BOOST_CHECK_EQUAL(string(real128("12345.6789")), string("12345.6789"));

   BOOST_CHECK_EQUAL( real128(uint64_t(-1)).to_uint64(), uint64_t(-1) );

   wdump( (ten)(two)(twenty) );
   wdump((real128("12345.6789")) );
   wdump( (ten/3*3) );
}

BOOST_AUTO_TEST_SUITE_END()

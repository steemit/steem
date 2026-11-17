#define BOOST_TEST_MODULE SaturationTest
#include <boost/test/included/unit_test.hpp>

#include <fc/saturation.hpp>
#include <cstdint>
#include <iostream>
#include <limits>

template< typename Signed, typename BiggerSigned >
int test_all()
{
   const Signed a = std::numeric_limits< Signed >::min();
   const Signed b = std::numeric_limits< Signed >::max();

   Signed x = a;
   while( true )
   {
      Signed y = a;
      while( true )
      {
         BiggerSigned z0big = BiggerSigned(x) + BiggerSigned(y);
         if( z0big < a ) z0big = a;
         if( z0big > b ) z0big = b;
         Signed z0 = Signed(z0big);
         Signed z1 = fc::signed_sat_add( x, y );
         if( z0 != z1 )
         {
            std::cout << "Addition error: " << int(x) << ", " << int(y) << std::endl;
            return 1;
         }

         z0big = BiggerSigned(x) - BiggerSigned(y);
         if( z0big < a ) z0big = a;
         if( z0big > b ) z0big = b;
         z0 = Signed(z0big);
         z1 = fc::signed_sat_sub( x, y );
         if( z0 != z1 )
         {
            std::cout << "Subtraction error: " << int(x) << ", " << int(y) << std::endl;
            return 1;
         }

         if( y == b ) break;
         ++y;
      }
      if( x == b ) break;
      ++x;
   }
   return 0;
}

int64_t sat_add_i64( int64_t a, int64_t b ) {
   return fc::signed_sat_add( a, b );
}

int64_t sat_sub_i64( int64_t a, int64_t b ) {
   return fc::signed_sat_sub( a, b );
}

BOOST_AUTO_TEST_CASE(test_saturation_int8)
{
   BOOST_REQUIRE((test_all<int8_t, int16_t>() == 0));
}

BOOST_AUTO_TEST_CASE(test_saturation_int16)
{
   BOOST_REQUIRE((test_all<int16_t, int32_t>() == 0));
}

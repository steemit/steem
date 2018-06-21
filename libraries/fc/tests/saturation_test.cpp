
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
         if( z0big < a )
            z0big = a;
         if( z0big > b )
            z0big = b;
         Signed z0 = Signed(z0big);
         Signed z1 = fc::signed_sat_add( x, y );

         if( z0 != z1 )
         {
            std::cout << "Addition does not work for " << x << ", " << y << std::endl;
            return 1;
         }

         z0big = BiggerSigned(x) - BiggerSigned(y);
         if( z0big < a )
            z0big = a;
         if( z0big > b )
            z0big = b;
         z0 = Signed(z0big);
         z1 = fc::signed_sat_sub( x, y );

         if( z0 != z1 )
         {
            std::cout << "Subtraction does not work for " << x << ", " << y << std::endl;
            return 1;
         }

         if( y == b )
            break;
         ++y;
      }
      if( x == b )
         break;
      ++x;
   }
   return 0;
}

int64_t sat_add_i64( int64_t a, int64_t b )
{
   return fc::signed_sat_add( a, b );
}

int64_t sat_sub_i64( int64_t a, int64_t b )
{
   return fc::signed_sat_sub( a, b );
}

int main( int argc, char** argv, char** envp )
{
   int result = test_all< int8_t, int16_t >();
   if( result )
      return 1;
   result = test_all< int16_t, int32_t >();
   if( result )
      return 1;

   return 0;
}

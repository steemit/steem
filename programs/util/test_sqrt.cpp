
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

#include <boost/multiprecision/integer.hpp>

#include <fc/uint128.hpp>

using fc::uint128_t;

uint8_t find_msb( const uint128_t& u )
{
   uint64_t x;
   uint8_t places;
   x      = (u.lo ? u.lo : 1);
   places = (u.hi ?   64 : 0);
   x      = (u.hi ? u.hi : x);
   return uint8_t( boost::multiprecision::detail::find_msb(x) + places );
}

uint64_t maybe_sqrt( uint64_t x )
{
   if( x <= 1 )
      return 0;
   assert( x <= std::numeric_limits< uint64_t >::max()/2 );

   uint8_t n = uint8_t( boost::multiprecision::detail::find_msb(x) );
   uint8_t b = n&1;
   uint64_t y = x + (uint64_t(1) << (n-(1-b)));
   y = y >> ((n >> 1)+b+1);
   return y;
}

uint64_t approx_sqrt( const uint128_t& x )
{
   if( (x.lo == 0) && (x.hi == 0) )
      return 0;

   uint8_t msb_x = find_msb(x);
   uint8_t msb_z = msb_x >> 1;

   uint128_t msb_x_bit = uint128_t(1) << msb_x;
   uint64_t  msb_z_bit = uint64_t (1) << msb_z;

   uint128_t mantissa_mask = msb_x_bit - 1;
   uint128_t mantissa_x = x & mantissa_mask;
   uint64_t mantissa_z_hi = (msb_x & 1) ? msb_z_bit : 0;
   uint64_t mantissa_z_lo = (mantissa_x >> (msb_x - msb_z)).lo;
   uint64_t mantissa_z = (mantissa_z_hi | mantissa_z_lo) >> 1;
   uint64_t result = msb_z_bit | mantissa_z;

   return result;
}

int main( int argc, char** argv, char** envp )
{
   uint128_t x = 0;
   while( true )
   {
      uint64_t y = approx_sqrt(x);
      std::cout << std::string(x) << " " << y << std::endl;
      //uint128_t new_x = x + (x >> 10) + 1;
      uint128_t new_x = x + (x / 1000) + 1;
      if( new_x < x )
         break;
      x = new_x;
   }
   return 0;
}

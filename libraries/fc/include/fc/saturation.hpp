#pragma once

#include <cstdint>
#include <limits>

namespace fc {

// Based loosely on http://locklessinc.com/articles/sat_arithmetic/

template< typename IntType >
struct corresponding_unsigned;

template<>
struct corresponding_unsigned< int8_t >
{
   typedef uint8_t type;
};

template<>
struct corresponding_unsigned< int16_t >
{
   typedef uint16_t type;
};

template<>
struct corresponding_unsigned< int32_t >
{
   typedef uint32_t type;
};

template<>
struct corresponding_unsigned< int64_t >
{
   typedef uint64_t type;
};

template< typename Signed >
Signed signed_sat_add( Signed x, Signed y )
{
   typedef typename corresponding_unsigned< Signed >::type u;
   u ux(x), uy(y);
   u res(ux+uy);

   // Compute saturated result sat, which is 0x7F... if x>=0, 0x80... otherwise
   u sat = (x >= 0) ? u( std::numeric_limits< Signed >::max() )
                    : u( std::numeric_limits< Signed >::min() );

   // If overflow occurs, the correct result is sat
   // If no overflow occurs, the correct result is res
   //
   // Overflow occurred if and only if the following two conditions occur:
   // (a) ux,uy are the same sign s
   // (b) res has a different sign than s
   //
   // But we know:
   // (a) iff ~(ux^uy)  has sign bit=1
   // (b) iff  (ux^res) has sign bit=1
   //
   // Therefore the sign bit of ~(ux^uy) & (uy ^ res) is 1 iff overflow occurred
   //
   return (Signed(~(ux^uy) & (ux^res)) < 0) ? sat : res;
}

template< typename Signed >
Signed signed_sat_sub( Signed x, Signed y )
{
   typedef typename corresponding_unsigned< Signed >::type u;
   u ux(x), uy(y);
   u res(ux-uy);

   // Compute saturated result sat, which is 0x7F... if x>=0, 0x80... otherwise
   u sat = (x >= 0) ? u( std::numeric_limits< Signed >::max() )
                    : u( std::numeric_limits< Signed >::min() );

   // If overflow occurs, the correct result is sat
   // If no overflow occurs, the correct result is res
   //
   // Overflow occurred if and only if the following two conditions occur:
   // (a) ux,uy are different signs
   // (b) res has a different sign than ux
   //
   // But we know:
   // (a) iff (ux^uy)  has sign bit=1
   // (b) iff (ux^res) has sign bit=1
   //
   // Therefore the sign bit of ~(ux^uy) & (uy ^ res) is 1 iff overflow occurred
   //
   return (Signed((ux^uy) & (ux^res)) < 0) ? sat : res;
}

}

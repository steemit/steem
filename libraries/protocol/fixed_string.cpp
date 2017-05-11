#include <steemit/protocol/fixed_string.hpp>

namespace fc
{
   uint128 endian_reverse( const uint128& u )
   {
      return uint128( boost::endian::endian_reverse( u.hi ), boost::endian::endian_reverse( u.lo ) );
   }
}

namespace std
{
   pair< fc::uint128_t, uint64_t > endian_reverse( const pair< fc::uint128_t, uint64_t > p )
   {
      return std::make_pair( fc::endian_reverse( p.first ), boost::endian::endian_reverse( p.second ) );
   }

   pair< fc::uint128_t, fc::uint128_t > endian_reverse( const pair< fc::uint128_t, fc::uint128_t > p )
   {
      return std::make_pair( fc::endian_reverse( p.first ), fc::endian_reverse( p.second ) );
   }
}

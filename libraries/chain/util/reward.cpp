
#include <steemit/chain/util/reward.hpp>

namespace steemit { namespace chain { namespace util {

uint128_t calculate_vshares( const uint128_t& rshares )
{
   uint128_t s = get_content_constant_s();
   uint128_t rshares_plus_s = rshares + s;
   return rshares_plus_s * rshares_plus_s - s * s;
}

} } }

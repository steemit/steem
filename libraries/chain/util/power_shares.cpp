#include <steem/chain/util/power_shares.hpp>

namespace steem { namespace chain { namespace util {

uint128_t get_regen_power_shares( const account_object& account, fc::time_point_sec now )
{
   uint32_t delta_time = ( now - account.last_power_shares_update ).to_seconds();
   uint128_t max_power_shares = get_effective_vesting_shares( account ) * STEEM_100_PERCENT;
   uint128_t regenerated_power_shares = ( max_power_shares * delta_time ) / STEEM_POWER_SHARES_REGENERATION_SECONDS;
   return std::min( account.power_shares + regenerated_power_shares, max_power_shares );
}

} } } //steem::chain

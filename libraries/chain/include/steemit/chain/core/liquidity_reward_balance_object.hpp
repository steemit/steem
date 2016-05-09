#pragma once

#include <steemit/chain/protocol/types.hpp>

#include <fc/time.hpp>
#include <fc/uint128.hpp>

#include <fc/reflect/reflect.hpp>

#include <cstdint>

namespace steemit { namespace chain { namespace core {

/**
 *  If last_update is greater than 1 week, then volume gets reset to 0
 *
 *  When a user is a maker, their volume increases
 *  When a user is a taker, their volume decreases
 *
 *  Every 1000 blocks, the account that has the highest volume_weight() is paid the maximum of
 *  1000 STEEM or 1000 * virtual_supply / (100*blocks_per_year) aka 10 * virtual_supply / blocks_per_year
 *
 *  After being paid volume gets reset to 0
 */
class liquidity_reward_balance_object
{
   public:
      account_id_type owner;
      int64_t         steem_volume = 0;
      int64_t         sbd_volume = 0;
      time_point_sec  last_update = fc::time_point_sec::min(); /// used to decay negative liquidity balances. block num

      /// this is the sort index
      uint128_t volume_weight()const { return steem_volume * sbd_volume * is_positive(); }

      inline int is_positive()const { return ( steem_volume > 0 && sbd_volume > 0 ) ? 1 : 0; }
};

} } }

FC_REFLECT(
   steemit::chain::core::liquidity_reward_balance_object,
   (owner)
   (steem_volume)
   (sbd_volume)
   (last_update)
   )

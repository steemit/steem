#pragma once

#include <steemit/chain/protocol/asset.hpp>

#include <fc/reflect/reflect.hpp>

#include <deque>

namespace steemit { namespace chain { namespace core {

/**
 *  This object gets updated once per hour, on the hour
 */

class feed_history_object
{
   public:
      price               current_median_history; ///< the current median of the price history, used as the base for convert operations
      std::deque<price>   price_history; ///< tracks this last week of median_feed one per hour
};

} } }

FC_REFLECT(
   steemit::chain::core::feed_history_object,
   (current_median_history)
   (price_history)
)

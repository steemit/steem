#pragma once

#include <steemit/chain/protocol/asset.hpp>

#include <fc/time.hpp>

#include <fc/reflect/reflect.hpp>

#include <cstdint>
#include <string>

namespace steemit { namespace chain { namespace core {

/**
 *  This object is used to track pending requests to convert sbd to steem
 */
class convert_request_object
{
   public:
      string         owner;
      uint32_t       requestid = 0; ///< id set by owner, the owner,requestid pair must be unique
      asset          amount;
      time_point_sec conversion_date; ///< at this time the feed_history_median_price * amount
};

} } }

FC_REFLECT(
   steemit::chain::core::convert_request_object,
   (owner)
   (requestid)
   (amount)
   (conversion_date)
)

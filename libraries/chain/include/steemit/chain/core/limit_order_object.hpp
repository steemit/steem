#pragma once

#include <steemit/chain/protocol/asset.hpp>

#include <fc/reflect/reflect.hpp>

#include <fc/time.hpp>

#include <string>
#include <utility>

namespace steemit { namespace chain { namespace core {

/**
 *  @brief an offer to sell a amount of a asset at a specified exchange rate by a certain time
 *  @ingroup object
 *  @ingroup protocol
 *  @ingroup market
 *
 *  This limit_order_objects are indexed by @ref expiration and is automatically deleted on the first block after expiration.
 */
class limit_order_object
{
   public:
      time_point_sec   created;
      time_point_sec   expiration;
      string           seller;
      uint32_t         orderid;
      share_type       for_sale; ///< asset id is sell_price.base.symbol
      price            sell_price;

      std::pair<asset_symbol_type,asset_symbol_type> get_market()const
      {
         return sell_price.base.symbol < sell_price.quote.symbol ?
             std::make_pair(sell_price.base.symbol, sell_price.quote.symbol) :
             std::make_pair(sell_price.quote.symbol, sell_price.base.symbol);
      }

      asset amount_for_sale()const   { return asset( for_sale, sell_price.base.symbol ); }
      asset amount_to_receive()const { return amount_for_sale() * sell_price; }
};

} } }

FC_REFLECT(
   steemit::chain::core::limit_order_object,
   (created)
   (expiration)
   (seller)
   (orderid)
   (for_sale)
   (sell_price)
)

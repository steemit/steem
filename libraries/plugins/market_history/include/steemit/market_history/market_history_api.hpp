#pragma once

#include <steemit/market_history/market_history_plugin.hpp>

#include <steemit/protocol/types.hpp>

#include <fc/api.hpp>

namespace steemit { namespace app {
   struct api_context;
} }

namespace steemit{ namespace market_history {

using chain::share_type;
using fc::time_point_sec;

namespace detail
{
   class market_history_api_impl;
}

struct market_ticker
{
   double      latest = 0;
   double      lowest_ask = 0;
   double      highest_bid = 0;
   double      percent_change = 0;
   asset       steem_volume = asset( 0 , STEEM_SYMBOL );
   asset       sbd_volume = asset( 0, SBD_SYMBOL );
};

struct market_volume
{
   asset       steem_volume = asset( 0, STEEM_SYMBOL );
   asset       sbd_volume = asset( 0, SBD_SYMBOL );
};

struct order
{
   double      price;
   share_type  steem;
   share_type  sbd;
};

struct order_book
{
   vector< order > bids;
   vector< order > asks;
};

struct market_trade
{
   time_point_sec date;
   asset          current_pays;
   asset          open_pays;
};

class market_history_api
{
   public:
      market_history_api( const steemit::app::api_context& ctx );

      void on_api_startup();

      /**
       * @brief Returns the market ticker for the internal SBD:STEEM market
       */
      market_ticker get_ticker() const;

      /**
       * @brief Returns the market volume for the past 24 hours
       */
      market_volume get_volume() const;

      /**
       * @brief Returns the current order book for the internal SBD:STEEM market.
       * @param limit The number of orders to have on each side of the order book. Maximum is 500
       */
      order_book get_order_book( uint32_t limit = 500 ) const;

      /**
       * @brief Returns the trade history for the internal SBD:STEEM market.
       * @param start The start time of the trade history.
       * @param end The end time of the trade history.
       * @param limit The number of trades to return. Maximum is 1000.
       * @return A list of completed trades.
       */
      std::vector< market_trade > get_trade_history( time_point_sec start, time_point_sec end, uint32_t limit = 1000 ) const;

      /**
       * @brief Returns the N most recent trades for the internal SBD:STEEM market.
       * @param limit The number of recent trades to return. Maximum is 1000.
       * @returns A list of completed trades.
       */
       std::vector< market_trade > get_recent_trades( uint32_t limit = 1000 ) const;

      /**
       * @brief Returns the market history for the internal SBD:STEEM market.
       * @param bucket_seconds The size of buckets the history is broken into. The bucket size must be configured in the plugin options.
       * @param start The start time to get market history.
       * @param end The end time to get market history
       * @return A list of market history buckets.
       */
      std::vector< bucket_object > get_market_history( uint32_t bucket_seconds, time_point_sec start, time_point_sec end ) const;

      /**
       * @brief Returns the bucket seconds being tracked by the plugin.
       */
      flat_set< uint32_t > get_market_history_buckets() const;

   private:
      std::shared_ptr< detail::market_history_api_impl > my;
};

} } // steemit::market_history

FC_REFLECT( steemit::market_history::market_ticker,
   (latest)(lowest_ask)(highest_bid)(percent_change)(steem_volume)(sbd_volume) );
FC_REFLECT( steemit::market_history::market_volume,
   (steem_volume)(sbd_volume) );
FC_REFLECT( steemit::market_history::order,
   (price)(steem)(sbd) );
FC_REFLECT( steemit::market_history::order_book,
   (bids)(asks) );
FC_REFLECT( steemit::market_history::market_trade,
   (date)(current_pays)(open_pays) );

FC_API( steemit::market_history::market_history_api,
   (get_ticker)
   (get_volume)
   (get_order_book)
   (get_trade_history)
   (get_recent_trades)
   (get_market_history)
   (get_market_history_buckets)
);
#pragma once
#include <steemit/plugins/json_rpc/utility.hpp>
#include <steemit/plugins/market_history/market_history_plugin.hpp>

#include <steemit/protocol/types.hpp>

#include <fc/optional.hpp>
#include <fc/variant.hpp>
#include <fc/vector.hpp>


namespace steemit { namespace plugins { namespace market_history_api {


using steemit::chain::share_type;
using steemit::chain::asset;
using fc::time_point_sec;
using json_rpc::void_type;


typedef void_type get_ticker_args;

struct get_ticker_return
{
   double      latest = 0;
   double      lowest_ask = 0;
   double      highest_bid = 0;
   double      percent_change = 0;
   asset       steem_volume = asset( 0 , STEEM_SYMBOL );
   asset       sbd_volume = asset( 0, SBD_SYMBOL );
};

typedef void_type get_volume_args;

struct get_volume_return
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

struct get_order_book_args
{
   uint32_t limit = 500;
};

struct get_order_book_return
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

struct get_trade_history_args
{
   time_point_sec start;
   time_point_sec end;
   uint32_t       limit = 1000;
};

struct get_trade_history_return
{
   std::vector< market_trade > trades;
};

struct get_recent_trades_args
{
   uint32_t limit = 1000;
};

typedef get_trade_history_return get_recent_trades_return;

struct get_market_history_args
{
   uint32_t       bucket_seconds;
   time_point_sec start;
   time_point_sec end;
};

struct get_market_history_return
{
   std::vector< market_history::bucket_object > buckets;
};

typedef void_type get_market_history_buckets_args;

struct get_market_history_buckets_return
{
   flat_set< uint32_t > bucket_sizes;
};


namespace detail { class market_history_api_impl; }


class market_history_api
{
   public:
      market_history_api();

      DECLARE_API( get_ticker )
      DECLARE_API( get_volume )
      DECLARE_API( get_order_book )
      DECLARE_API( get_trade_history )
      DECLARE_API( get_recent_trades )
      DECLARE_API( get_market_history )
      DECLARE_API( get_market_history_buckets )

   private:
      std::shared_ptr< detail::market_history_api_impl > my;
};

} } } // steemit::plugins::market_history_api

FC_REFLECT( steemit::plugins::market_history_api::get_ticker_return,
            (latest)(lowest_ask)(highest_bid)(percent_change)(steem_volume)(sbd_volume) )

FC_REFLECT( steemit::plugins::market_history_api::get_volume_return,
            (steem_volume)(sbd_volume) )

FC_REFLECT( steemit::plugins::market_history_api::order,
            (price)(steem)(sbd) )

FC_REFLECT( steemit::plugins::market_history_api::get_order_book_args,
            (limit) )

FC_REFLECT( steemit::plugins::market_history_api::get_order_book_return,
            (bids)(asks) )

FC_REFLECT( steemit::plugins::market_history_api::market_trade,
            (date)(current_pays)(open_pays) )

FC_REFLECT( steemit::plugins::market_history_api::get_trade_history_args,
            (start)(end)(limit) )

FC_REFLECT( steemit::plugins::market_history_api::get_trade_history_return,
            (trades) )

FC_REFLECT( steemit::plugins::market_history_api::get_recent_trades_args,
            (limit) )

FC_REFLECT( steemit::plugins::market_history_api::get_market_history_args,
            (bucket_seconds)(start)(end) )

FC_REFLECT( steemit::plugins::market_history_api::get_market_history_return,
            (buckets) )

FC_REFLECT( steemit::plugins::market_history_api::get_market_history_buckets_return,
            (bucket_sizes) )

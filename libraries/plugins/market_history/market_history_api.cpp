#include <steemit/market_history/market_history_api.hpp>

namespace steemit { namespace market_history {

namespace detail
{

class market_history_api_impl
{
   public:
      market_history_api_impl( steemit::app::application& _app )
         :app( _app ) {}

      market_ticker get_ticker() const;
      market_volume get_volume() const;
      order_book get_order_book( uint32_t limit ) const;
      vector< market_trade > get_trade_history( time_point_sec start, time_point_sec end, uint32_t limit ) const;
      vector< bucket_object > get_market_history( uint32_t bucket_seconds, time_point_sec start, time_point_sec end ) const;
      flat_set< uint32_t > get_market_history_buckets() const;

      steemit::app::application& app;
};

} // detail

market_ticker market_history_api::get_ticker() const
{
   return my->get_ticker();
}

market_volume market_history_api::get_volume() const
{
   return my->get_volume();
}

order_book market_history_api::get_order_book( uint32_t limit ) const
{
   return my->get_order_book( limit );
}

std::vector< market_trade > market_history_api::get_trade_history( time_point_sec start, time_point_sec end, uint32_t limit ) const
{
   return my->get_trade_history( start, end, limit );
}

std::vector< bucket_object > market_history_api::get_market_history( uint32_t bucket_seconds, time_point_sec start, time_point_sec end ) const
{
   return my->get_market_history( bucket_seconds, start, end );
}

flat_set< uint32_t > market_history_api::get_market_history_buckets() const
{
   return my->get_market_history_buckets();
}

} } // steemit::market_history
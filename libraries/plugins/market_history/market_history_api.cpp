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

market_ticker market_history_api_impl::get_ticker() const
{

}

market_volume market_history_api_impl::get_volume() const
{
   auto db = app.chain_database();
   const auto& bucket_idx = db->get_index_type< bucket_index >().indices().get< by_bucket >();
   auto itr = bucket_idx.lower_bound( boost::make_tuple( 0, db->head_block_time() - 86400 ) );
   uint32_t bucket_size;

   market_volume result;

   if( itr != bucket_idx.end() )
      bucket_size = itr->seconds;

   while( itr != bucket_idx.end() && itr->seconds == bucket_size )
   {
      result.steem_volume += itr->steem_volume;
      result.sbd_volume += itr->sbd_volume;

      itr++;
   }

   return result;
}

order_book market_history_api_impl::get_order_book( uint32_t limit ) const
{
   FC_ASSERT( limit <= 50 );
}

std::vector< market_trade > market_history_api_impl::get_trade_history( time_point_sec start, time_point_sec end, uint32_t limit ) const
{
   FC_ASSERT( limit <= 100 );
   const auto& bucket_idx = app.chain_database()->get_index_type< order_history_index >().indices().get< by_time >();
   auto itr = bucket_idx.lower_bound( start );

   std::vector< market_trade > result;

   while( itr != bucket_idx.end() && itr->time <= end && result.size() < limit )
   {
      market_trade trade;
      trade.date = itr->time;

      // Only return one side of the trade
      if( itr->op.pays.symbol == STEEM_SYMBOL )
      {
         trade.steem = itr->op.pays.amount;
         trade.sbd = itr->op.receives.amount;
         result.push_back( trade );
      }

      itr++;
   }

   return result;
}

std::vector< bucket_object > market_history_api_impl::get_market_history( uint32_t bucket_seconds, time_point_sec start, time_point_sec end ) const
{

}

chain::flat_set< uint32_t > market_history_api_impl::get_market_history_buckets() const
{
   auto buckets = app.get_plugin< market_history_plugin >( MARKET_HISTORY_PLUGIN_NAME )->get_tracked_buckets();
   return buckets;
}

} // detail

market_history_api::market_history_api( const steemit::app::api_context& ctx )
{
   my = std::make_shared< detail::market_history_api_impl >( ctx.app );
}

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
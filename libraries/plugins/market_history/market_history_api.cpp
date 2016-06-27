#include <steemit/market_history/market_history_api.hpp>

#include <steemit/chain/steem_objects.hpp>

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
   market_ticker result;

   auto trades = get_trade_history( time_point_sec::maximum(), time_point_sec::min(), 1 );
   if( trades.size() )
      result.latest = ( asset( trades[0].steem, STEEM_SYMBOL ) / asset( trades[0].sbd, SBD_SYMBOL ) ).to_real();

   auto db = app.chain_database();
   const auto& bucket_idx = db->get_index_type< bucket_index >().indices().get< by_bucket >();
   auto itr = bucket_idx.lower_bound( boost::make_tuple( 0, db->head_block_time() - 86400 ) );

   if( itr != bucket_idx.end() && trades.size() )
   {
      auto open = ( asset( itr->open_steem, STEEM_SYMBOL ) / asset( itr->open_sbd, SBD_SYMBOL ) ).to_real();
      result.percent_change = ( result.latest - open ) / open;
   }

   auto orders = get_order_book( 1 );
   if( orders.bids.size() )
      result.highest_bid = orders.bids[0].price;
   if( orders.asks.size() )
      result.lowest_ask = orders.asks[0].price;

   auto volume = get_volume();
   result.steem_volume = volume.steem_volume;
   result.sbd_volume = volume.sbd_volume;

   return result;
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

   const auto& order_idx = app.chain_database()->get_index_type< steemit::chain::limit_order_index >().indices().get< steemit::chain::by_price >();
   auto itr = order_idx.upper_bound( std::make_tuple( price( asset( ~0, SBD_SYMBOL ), asset( 1, STEEM_SYMBOL ) ) ) );
   auto end = order_idx.lower_bound( std::make_tuple( price( asset( 0, SBD_SYMBOL ), asset( 1, STEEM_SYMBOL ) ) ) );

   order_book result;

   while( itr != end && itr->sell_price.base.symbol == SBD_SYMBOL && result.bids.size() < limit )
   {
      order cur;
      cur.price = itr->sell_price.quote.to_real() / itr->sell_price.base.to_real();
      cur.steem = ( asset( itr->for_sale, SBD_SYMBOL ) * itr->sell_price ).amount;
      cur.sbd = itr->for_sale;
      result.bids.push_back( cur );
      itr--;
   }

   itr = order_idx.upper_bound( std::make_tuple( price( asset( ~0, STEEM_SYMBOL ), asset( 1, SBD_SYMBOL ) ) ) );
   end = order_idx.lower_bound( std::make_tuple( price( asset( 0, STEEM_SYMBOL ), asset( 1, SBD_SYMBOL ) ) ) );

   while( itr != end && itr->sell_price.base.symbol == STEEM_SYMBOL && result.asks.size() < limit )
   {
      order cur;
      cur.price = itr->sell_price.to_real();
      cur.steem = itr->for_sale;
      cur.sbd = ( asset( itr->for_sale, STEEM_SYMBOL ) * itr->sell_price ).amount;
      result.asks.push_back( cur );
      itr--;
   }

   return result;
}

std::vector< market_trade > market_history_api_impl::get_trade_history( time_point_sec start, time_point_sec end, uint32_t limit ) const
{
   FC_ASSERT( limit <= 100 );
   const auto& bucket_idx = app.chain_database()->get_index_type< order_history_index >().indices().get< by_time >();
   auto itr = bucket_idx.lower_bound( start );

   std::vector< market_trade > result;

   while( itr != bucket_idx.end() && itr->time <= end && result.size() < limit )
   {
      // Only return one side of the trade
      if( itr->op.pays.symbol == STEEM_SYMBOL )
      {
         market_trade trade;
         trade.date = itr->time;
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
   const auto& bucket_idx = app.chain_database()->get_index_type< bucket_index >().indices().get< by_bucket >();
   auto itr = bucket_idx.lower_bound( boost::make_tuple( bucket_seconds, start ) );

   std::vector< bucket_object > result;

   while( itr != bucket_idx.end() && itr->seconds == bucket_seconds && itr->open < end )
   {
      result.push_back( *itr );

      itr++;
   }

   return result;
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

void market_history_api::on_api_startup() {}

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
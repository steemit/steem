#include <steemit/plugins/market_history/market_history_plugin.hpp>

#include <steemit/chain/database.hpp>
#include <steemit/chain/index.hpp>
#include <steemit/chain/operation_notification.hpp>

#include <fc/io/json.hpp>

namespace steemit { namespace plugins { namespace market_history {

namespace detail {

using steemit::protocol::fill_order_operation;

class market_history_plugin_impl
{
   public:
      market_history_plugin_impl() :
         _db( appbase::app().get_plugin< steemit::plugins::chain::chain_plugin >().db() ) {}
      virtual ~market_history_plugin_impl() {}

      /**
       * This method is called as a callback after a block is applied
       * and will process/index all operations that were applied in the block.
       */
      void update_market_histories( const operation_notification& o );

      steemit::chain::database&     _db;
      flat_set<uint32_t>            _tracked_buckets = flat_set<uint32_t>  { 15, 60, 300, 3600, 86400 };
      int32_t                       _maximum_history_per_bucket_size = 1000;
      boost::signals2::connection   post_apply_connection;
};

void market_history_plugin_impl::update_market_histories( const operation_notification& o )
{
   if( o.op.which() == operation::tag< fill_order_operation >::value )
   {
      fill_order_operation op = o.op.get< fill_order_operation >();

      const auto& bucket_idx = _db.get_index< bucket_index >().indices().get< by_bucket >();

      _db.create< order_history_object >( [&]( order_history_object& ho )
      {
         ho.time = _db.head_block_time();
         ho.op = op;
      });

      if( !_maximum_history_per_bucket_size ) return;
      if( !_tracked_buckets.size() ) return;

      for( auto bucket : _tracked_buckets )
      {
         auto cutoff = _db.head_block_time() - fc::seconds( bucket * _maximum_history_per_bucket_size );

         auto open = fc::time_point_sec( ( _db.head_block_time().sec_since_epoch() / bucket ) * bucket );
         auto seconds = bucket;

         auto itr = bucket_idx.find( boost::make_tuple( seconds, open ) );
         if( itr == bucket_idx.end() )
         {
            _db.create< bucket_object >( [&]( bucket_object& b )
            {
               b.open = open;
               b.seconds = bucket;

               if( op.open_pays.symbol == STEEM_SYMBOL )
               {
                  b.high_steem = op.open_pays.amount;
                  b.high_sbd = op.current_pays.amount;
                  b.low_steem = op.open_pays.amount;
                  b.low_sbd = op.current_pays.amount;
                  b.open_steem = op.open_pays.amount;
                  b.open_sbd = op.current_pays.amount;
                  b.close_steem = op.open_pays.amount;
                  b.close_sbd = op.current_pays.amount;
                  b.steem_volume = op.open_pays.amount;
                  b.sbd_volume = op.current_pays.amount;
               }
               else
               {
                  b.high_steem = op.current_pays.amount;
                  b.high_sbd = op.open_pays.amount;
                  b.low_steem = op.current_pays.amount;
                  b.low_sbd = op.open_pays.amount;
                  b.open_steem = op.current_pays.amount;
                  b.open_sbd = op.open_pays.amount;
                  b.close_steem = op.current_pays.amount;
                  b.close_sbd = op.open_pays.amount;
                  b.steem_volume = op.current_pays.amount;
                  b.sbd_volume = op.open_pays.amount;
               }
            });
         }
         else
         {
            _db.modify( *itr, [&]( bucket_object& b )
            {
               if( op.open_pays.symbol == STEEM_SYMBOL )
               {
                  b.steem_volume += op.open_pays.amount;
                  b.sbd_volume += op.current_pays.amount;
                  b.close_steem = op.open_pays.amount;
                  b.close_sbd = op.current_pays.amount;

                  if( b.high() < price( op.current_pays, op.open_pays ) )
                  {
                     b.high_steem = op.open_pays.amount;
                     b.high_sbd = op.current_pays.amount;
                  }

                  if( b.low() > price( op.current_pays, op.open_pays ) )
                  {
                     b.low_steem = op.open_pays.amount;
                     b.low_sbd = op.current_pays.amount;
                  }
               }
               else
               {
                  b.steem_volume += op.current_pays.amount;
                  b.sbd_volume += op.open_pays.amount;
                  b.close_steem = op.current_pays.amount;
                  b.close_sbd = op.open_pays.amount;

                  if( b.high() < price( op.open_pays, op.current_pays ) )
                  {
                     b.high_steem = op.current_pays.amount;
                     b.high_sbd = op.open_pays.amount;
                  }

                  if( b.low() > price( op.open_pays, op.current_pays ) )
                  {
                     b.low_steem = op.current_pays.amount;
                     b.low_sbd = op.open_pays.amount;
                  }
               }
            });

            if( _maximum_history_per_bucket_size > 0 )
            {
               open = fc::time_point_sec();
               itr = bucket_idx.lower_bound( boost::make_tuple( seconds, open ) );

               while( itr->seconds == seconds && itr->open < cutoff )
               {
                  auto old_itr = itr;
                  ++itr;
                  _db.remove( *old_itr );
               }
            }
         }
      }
   }
}

} // detail

market_history_plugin::market_history_plugin() {}
market_history_plugin::~market_history_plugin() {}

void market_history_plugin::set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg
)
{
   cfg.add_options()
         ("market-history-bucket-size", boost::program_options::value<string>()->default_value("[15,60,300,3600,86400]"),
           "Track market history by grouping orders into buckets of equal size measured in seconds specified as a JSON array of numbers")
         ("market-history-buckets-per-size", boost::program_options::value<uint32_t>()->default_value(5760),
           "How far back in time to track history for each bucket size, measured in the number of buckets (default: 5760)")
         ;
}

void market_history_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   try
   {
      ilog( "market_history: plugin_initialize() begin" );
      _my = std::make_unique< detail::market_history_plugin_impl >();

      _my->post_apply_connection = _my->_db.post_apply_operation.connect( [&]( const operation_notification& o ){ _my->update_market_histories( o ); } );
      add_plugin_index< bucket_index        >( _my->_db );
      add_plugin_index< order_history_index >( _my->_db );

      if( options.count("bucket-size" ) )
      {
         std::string buckets = options["bucket-size"].as< string >();
         _my->_tracked_buckets = fc::json::from_string( buckets ).as< flat_set< uint32_t > >();
      }
      if( options.count("history-per-size" ) )
         _my->_maximum_history_per_bucket_size = options["history-per-size"].as< uint32_t >();

      wlog( "bucket-size ${b}", ("b", _my->_tracked_buckets) );
      wlog( "history-per-size ${h}", ("h", _my->_maximum_history_per_bucket_size) );

      ilog( "market_history: plugin_initialize() end" );
   } FC_CAPTURE_AND_RETHROW()
}

void market_history_plugin::plugin_startup() {}

void market_history_plugin::plugin_shutdown()
{
   _my->_db.disconnect_signal( _my->post_apply_connection );
}

flat_set< uint32_t > market_history_plugin::get_tracked_buckets() const
{
   return _my->_tracked_buckets;
}

uint32_t market_history_plugin::get_max_history_per_bucket() const
{
   return _my->_maximum_history_per_bucket_size;
}

} } } // steemit::plugins::market_history

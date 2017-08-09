#include <steemit/market_history/market_history_api.hpp>

#include <steemit/chain/database.hpp>
#include <steemit/chain/index.hpp>
#include <steemit/chain/operation_notification.hpp>

namespace steemit { namespace market_history {

namespace detail {

using steemit::protocol::fill_order_operation;

class market_history_plugin_impl
{
   public:
      market_history_plugin_impl( market_history_plugin& plugin )
         :_self( plugin ) {}
      virtual ~market_history_plugin_impl() {}

      /**
       * This method is called as a callback after a block is applied
       * and will process/index all operations that were applied in the block.
       */
      void update_market_histories( const operation_notification& o );

      market_history_plugin& _self;
      flat_set<uint32_t>     _tracked_buckets = flat_set<uint32_t>  { 15, 60, 300, 3600, 86400 };
      int32_t                _maximum_history_per_bucket_size = 1000;
};

void market_history_plugin_impl::update_market_histories( const operation_notification& o )
{
   if( o.op.which() == operation::tag< fill_order_operation >::value )
   {
      fill_order_operation op = o.op.get< fill_order_operation >();

      auto& db = _self.database();
      const auto& bucket_idx = db.get_index< bucket_index >().indices().get< by_bucket >();

      db.create< order_history_object >( [&]( order_history_object& ho )
      {
         ho.time = db.head_block_time();
         ho.op = op;
      });

      if( !_maximum_history_per_bucket_size ) return;
      if( !_tracked_buckets.size() ) return;

      for( auto bucket : _tracked_buckets )
      {
         auto cutoff = db.head_block_time() - fc::seconds( bucket * _maximum_history_per_bucket_size );

         auto open = fc::time_point_sec( ( db.head_block_time().sec_since_epoch() / bucket ) * bucket );
         auto seconds = bucket;

         auto itr = bucket_idx.find( boost::make_tuple( seconds, open ) );
         if( itr == bucket_idx.end() )
         {
            db.create< bucket_object >( [&]( bucket_object& b )
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
            db.modify( *itr, [&]( bucket_object& b )
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
                  db.remove( *old_itr );
               }
            }
         }
      }
   }
}

} // detail

market_history_plugin::market_history_plugin( application* app )
   : plugin( app ), _my( new detail::market_history_plugin_impl( *this ) ) {}
market_history_plugin::~market_history_plugin() {}

void market_history_plugin::plugin_set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg
)
{
   cli.add_options()
         ("market-history-bucket-size", boost::program_options::value<string>()->default_value("[15,60,300,3600,86400]"),
           "Track market history by grouping orders into buckets of equal size measured in seconds specified as a JSON array of numbers")
         ("market-history-buckets-per-size", boost::program_options::value<uint32_t>()->default_value(5760),
           "How far back in time to track history for each bucket size, measured in the number of buckets (default: 5760)")
         ;
   cfg.add(cli);
}

void market_history_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   try
   {
      ilog( "market_history: plugin_initialize() begin" );
      chain::database& db = database();

      db.post_apply_operation.connect( [&]( const operation_notification& o ){ _my->update_market_histories( o ); } );
      add_plugin_index< bucket_index        >(db);
      add_plugin_index< order_history_index >(db);

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

void market_history_plugin::plugin_startup()
{
   ilog( "market_history plugin: plugin_startup() begin" );

   app().register_api_factory< market_history_api >( "market_history_api" );

   ilog( "market_history plugin: plugin_startup() end" );
}

flat_set< uint32_t > market_history_plugin::get_tracked_buckets() const
{
   return _my->_tracked_buckets;
}

uint32_t market_history_plugin::get_max_history_per_bucket() const
{
   return _my->_maximum_history_per_bucket_size;
}

} } // steemit::market_history

STEEMIT_DEFINE_PLUGIN( market_history, steemit::market_history::market_history_plugin )

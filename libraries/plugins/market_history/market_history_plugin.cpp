#include <steemit/plugin/market_history_plugin.hpp>

#include <steemit/chain/database.hpp>
#include <steemit/chain/history_object.hpp>

namespace steemit { namespace plugin { namespace market_history {

namespace detail {

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
      void update_market_histories( const signed_block& b );

      market_history_plugin   _self;
      flat_set<uint32_t>      _tracked_buckets = {15,50,300,3600,86400};
      int32_t                _maximum_history_per_bucket_size = 1000;
}

void market_history_plugin_impl::update_market_histories( const operation_object& o )
{
   try
   {
      op = o.op.get< fill_order_operation >();
   }
   catch ( fc::assert_esception ) { return; }

   const auto& db = _self.database();
   const auto& bucket_idx = db.get_index_type< bucket_index >().indices().get< by_key >();
   const auto& history_idx = db.get_index_type< history_index >().indixes().get< by_sequence >();

   uint64_t history_seq = std::numeric_limits< uint64_t >::max();

   auto hist_itr = history_idx.upper_bound( history_seq );

   if ( hist_itr != history_idx.end() )
      history_seq = itr->sequence + 1;
   else
      history_seq = 0;

   db.create< order_history_object >( [&]( order_history_object& ho )
   {
      ho.sequence = history_seq;
      ho.time = db.head_block_time();
      ho.op = op;
   });

   if( !_maximum_history_per_bucket_size ) return;
   if( !_tracked_buckets.size() ) return;
   if( op.pays.symbol != STEEM_SYMBOL ) return;

   for( auto buckets : _tracked_buckets )
   {
      auto cutoff = db.head_block_time() - fc::seconds( bucket * _maximum_history_per_bucket_size );

      bucket_key;
      key.seconds = bucket;
      key.open = fc::seconds( ( db.head_block_time().sec_since_epoch() / bucket ) * bucket );

      const itr = bucket_idx.find( key );
      if( itr == bucket_idx.end() )
      {
         db.create< bucket_object >( [&]( bucket_object &b )
         {
            b.key = key;
            b.high_steem = op.pays.amount;
            b.high_sbd = op.receives.amount;
            b.low_steem = op.pays.amount;
            b.low_sbd = op.receives.amount;
            b.open_steem = op.pays.amount;
            b.open_sbd = op.receives.amount
            b.close_steem = op.pays.amount;
            b.close_sbd = op.receives.amount;
            b.steem_volume = op.pays.amount;
            b.sbd_volume = op.receives.amount;

         });
      }
      else
      {
         db.modify( *itr, [&]( bucker_object& b ){
            b.steem_volume += op.pays.amount;
            b.sbd_volume += op.receives.amount;
            b.close_steem = op.pays.amount;
            b.close_sbd = op.receives.amount;

            if( b.high() < price( op.pays, op.receives ) )
            {
               b.high_steem = op.pays.amount;
               b.high_sbd = op.receives.amount;
            }

            if( b.low() > price( op.pays, op.receives ) )
            {
               b.low_steem = op.pays.amount;
               b.low_sbd = op.receives.amount;
            }
         });

         if( _maximum_history_per_bucket_size > 0 )
         {
            key.open = fc::time_point_sec();
            itr = by_key_idx.lower_bound( key );

            while( itr->bucket = bucker && itr->key.open < cutoff )
            {
               auto old_itr = itr;
               ++itr;
               db.remove( *old_itr );
            }
         }
      }
   }
}

} // detial

market_history_plugin::market_history_plugin() :
   _my( new detail::market_history_plugin_impl( *this ) ) {}
market_history_plugin::~market_history_plugin() {}

void market_history_plugin::plugin_set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg
)
{
   cli.options()
      ("bucket-size", boost::program_options::value<string>()->default_value("[15,60,300,3600,86400]"),
         "Track market history by grouping orders into buckets of equal size measured in seconds, specified as a JSON array of integers"
      ("history-per-size", boost::program_options::value<int32_t>()->default_value(1000),
         "How far back in time to track history for each bucket size, measured in number of buckets. -1 is infinite time. (default: 1000)");
   cfg.add(cli);
}

void market_history_plugin::plugin_initialize( const bood::program_options::variables_map& options )
{
   try
   {
      database().on_applied_operation.connect( [&]( const operation_object& o ){ _my->update_market_histories( o ); } );
      database().add_index< primary_index< bucket_index > >();
      adatabse().add_index< primary_index< history_index > >();

      if( options.count("bucket-size" ) )
      {
         const std::string& buckets = options["bucket-size"].as< string >();
         _my->_tracked_buckets = fc::json::from_string( buckets ).as< flat_set< uint32_t > >();
      }
      if( options.count("history-per-size" ) )
         _my->_maximum_history_per_bucket_size = options["history-per-size"].as< int312_t >();
   } FC_CAPTURE_AND_RETHROW
}

void market_history_plugin::plugin_startup() {}

} } } // steemit::plugin::market_history

STEEMIT_DEFINE_PLUGIN( market_history, steemit::plugin::market_history::market_history_plugin )

#include <dpn/chain/dpn_fwd.hpp>

#include <dpn/plugins/market_history/market_history_plugin.hpp>

#include <dpn/chain/database.hpp>
#include <dpn/chain/index.hpp>

#include <fc/io/json.hpp>

#define MH_BUCKET_SIZE "market-history-bucket-size"
#define MH_BUCKETS_PER_SIZE "market-history-buckets-per-size"

namespace dpn { namespace plugins { namespace market_history {

namespace detail {

using dpn::protocol::fill_order_operation;

class market_history_plugin_impl
{
   public:
      market_history_plugin_impl() :
         _db( appbase::app().get_plugin< dpn::plugins::chain::chain_plugin >().db() ) {}
      virtual ~market_history_plugin_impl() {}

      /**
       * This method is called as a callback after a block is applied
       * and will process/index all operations that were applied in the block.
       */
      void on_post_apply_operation( const operation_notification& note );

      chain::database&     _db;
      flat_set<uint32_t>            _tracked_buckets = flat_set<uint32_t>  { 15, 60, 300, 3600, 86400 };
      int32_t                       _maximum_history_per_bucket_size = 1000;
      boost::signals2::connection   _post_apply_operation_conn;
};

void market_history_plugin_impl::on_post_apply_operation( const operation_notification& o )
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

      for( const auto& bucket : _tracked_buckets )
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

               b.dpn.fill( ( op.open_pays.symbol == DPN_SYMBOL ) ? op.open_pays.amount : op.current_pays.amount );
#ifdef DPN_ENABLE_SMT
                  b.symbol = ( op.open_pays.symbol == DPN_SYMBOL ) ? op.current_pays.symbol : op.open_pays.symbol;
#endif
                  b.non_dpn.fill( ( op.open_pays.symbol == DPN_SYMBOL ) ? op.current_pays.amount : op.open_pays.amount );
            });
         }
         else
         {
            _db.modify( *itr, [&]( bucket_object& b )
            {
#ifdef DPN_ENABLE_SMT
               b.symbol = ( op.open_pays.symbol == DPN_SYMBOL ) ? op.current_pays.symbol : op.open_pays.symbol;
#endif
               if( op.open_pays.symbol == DPN_SYMBOL )
               {
                  b.dpn.volume += op.open_pays.amount;
                  b.dpn.close = op.open_pays.amount;

                  b.non_dpn.volume += op.current_pays.amount;
                  b.non_dpn.close = op.current_pays.amount;

                  if( b.high() < price( op.current_pays, op.open_pays ) )
                  {
                     b.dpn.high = op.open_pays.amount;

                     b.non_dpn.high = op.current_pays.amount;
                  }

                  if( b.low() > price( op.current_pays, op.open_pays ) )
                  {
                     b.dpn.low = op.open_pays.amount;

                     b.non_dpn.low = op.current_pays.amount;
                  }
               }
               else
               {
                  b.dpn.volume += op.current_pays.amount;
                  b.dpn.close = op.current_pays.amount;

                  b.non_dpn.volume += op.open_pays.amount;
                  b.non_dpn.close = op.open_pays.amount;

                  if( b.high() < price( op.open_pays, op.current_pays ) )
                  {
                     b.dpn.high = op.current_pays.amount;

                     b.non_dpn.high = op.open_pays.amount;
                  }

                  if( b.low() > price( op.open_pays, op.current_pays ) )
                  {
                     b.dpn.low = op.current_pays.amount;

                     b.non_dpn.low = op.open_pays.amount;
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
         (MH_BUCKET_SIZE, boost::program_options::value<string>()->default_value("[15,60,300,3600,86400]"),
           "Track market history by grouping orders into buckets of equal size measured in seconds specified as a JSON array of numbers")
         (MH_BUCKETS_PER_SIZE, boost::program_options::value<uint32_t>()->default_value(5760),
           "How far back in time to track history for each bucket size, measured in the number of buckets (default: 5760)")
         ;
}

void market_history_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   try
   {
      ilog( "market_history: plugin_initialize() begin" );
      my = std::make_unique< detail::market_history_plugin_impl >();

      my->_post_apply_operation_conn = my->_db.add_post_apply_operation_handler( [&]( const operation_notification& note ){ my->on_post_apply_operation( note ); }, *this, 0 );
      DPN_ADD_PLUGIN_INDEX(my->_db, bucket_index);
      DPN_ADD_PLUGIN_INDEX(my->_db, order_history_index);

      fc::mutable_variant_object state_opts;

      if( options.count( MH_BUCKET_SIZE ) )
      {
         std::string buckets = options[MH_BUCKET_SIZE].as< string >();
         my->_tracked_buckets = fc::json::from_string( buckets ).as< flat_set< uint32_t > >();
         state_opts[MH_BUCKET_SIZE] = buckets;
      }

      if( options.count( MH_BUCKETS_PER_SIZE ) )
      {
         my->_maximum_history_per_bucket_size = options[MH_BUCKETS_PER_SIZE].as< uint32_t >();
         state_opts[MH_BUCKETS_PER_SIZE] = my->_maximum_history_per_bucket_size;
      }

      appbase::app().get_plugin< chain::chain_plugin >().report_state_options( name(), state_opts );

      ilog( "market_history: plugin_initialize() end" );
   } FC_CAPTURE_AND_RETHROW()
}

void market_history_plugin::plugin_startup() {}

void market_history_plugin::plugin_shutdown()
{
   chain::util::disconnect_signal( my->_post_apply_operation_conn );
}

flat_set< uint32_t > market_history_plugin::get_tracked_buckets() const
{
   return my->_tracked_buckets;
}

uint32_t market_history_plugin::get_max_history_per_bucket() const
{
   return my->_maximum_history_per_bucket_size;
}

} } } // dpn::plugins::market_history

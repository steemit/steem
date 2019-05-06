
#include <steem/chain/steem_fwd.hpp>

#include <steem/plugins/transaction_status/transaction_status_plugin.hpp>
#include <steem/plugins/transaction_status/transaction_status_objects.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/index.hpp>
#include <steem/protocol/config.hpp>

#include <fc/io/json.hpp>

#define TRANSACTION_STATUS_BLOCK_DEPTH_KEY            "transaction-status-block-depth"
#define TRANSACTION_STATUS_DEFAULT_BLOCK_DEPTH        64000

#define TRANSACTION_STATUS_TRACK_AFTER_KEY            "transaction-status-track-after-block"
#define TRANSACTION_STATUS_DEFAULT_TRACK_AFTER        0

#define TRANSACTION_STATUS_REBUILD_STATE_KEY          "transaction-status-rebuild-state"

/*
 *                             window of uncertainty              trackable
 *                          .-------------------------. .---------------------------.
 *                         |                           |                             |
 *   <- - - - - - - - - - [*] - - - - - - - - - - - - [*] - - - - - - - - - - - - - [*]
 *                        /                            |                              \
 *               actual block depth            nominal block depth                head block
 *
 * - Within the window of uncertainy, if the transaction is found we will return the status
 *      If the transaction is not found and an expiration is provided, we will return `too_old`
 *
 * - Within the trackable range, if the transaction is found we will return the status
 *      If the transaction is not found and an expiration is provided we will return the expiration status
 *
 * - Nominal values are values provided by the user
 *
 * - Actual values are calculated and used to determine when tracking needs to begin
 *      see `plugin_initialize`
 */

namespace steem { namespace plugins { namespace transaction_status {

namespace detail {

class transaction_status_impl
{
public:
   transaction_status_impl() : _db( appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db() ) {}
   virtual ~transaction_status_impl() {}

   void on_post_apply_transaction( const transaction_notification& note );
   void on_post_apply_block( const block_notification& note );

   chain::database&              _db;
   uint32_t                      nominal_block_depth = 0;       //!< User provided block-depth
   uint32_t                      nominal_track_after_block = 0; //!< User provided track-after-block
   uint32_t                      actual_block_depth = 0;        //!< Calculated block-depth
   uint32_t                      actual_track_after_block = 0;  //!< Calculated track-after-block
   bool                          tracking = false;
   bool                          rebuild_state_flag = false;
   boost::signals2::connection   post_apply_transaction_connection;
   boost::signals2::connection   post_apply_block_connection;
   bool                          state_is_valid();
   void                          rebuild_state();
   uint32_t                      get_earliest_tracked_block_num();

private:
   fc::optional< transaction_id_type >  get_earliest_transaction_in_range( const uint32_t first_block_num, const uint32_t last_block_num );
   fc::optional< transaction_id_type >  get_latest_transaction_in_range( const uint32_t first_block_num, const uint32_t last_block_num );
};

void transaction_status_impl::on_post_apply_transaction( const transaction_notification& note )
{
   if ( tracking )
      _db.create< transaction_status_object >( [&]( transaction_status_object& obj )
      {
         obj.transaction_id = note.transaction_id;
      } );
}

void transaction_status_impl::on_post_apply_block( const block_notification& note )
{
   if ( tracking )
   {
      // Update all status objects with the transaction current block number
      for ( const auto& e : note.block.transactions )
      {
         const auto& tx_status_obj = _db.get< transaction_status_object, by_trx_id >( e.id() );

         _db.modify( tx_status_obj, [&] ( transaction_status_object& obj )
         {
            obj.block_num = note.block_num;
         } );
      }

      // Remove elements from the index that are deemed too old for tracking
      if ( note.block_num > actual_block_depth )
      {
         uint32_t obsolete_block = note.block_num - actual_block_depth;
         const auto& idx = _db.get_index< transaction_status_index >().indices().get< by_block_num >();

         auto itr = idx.begin();
         while ( itr != idx.end() && itr->block_num <= obsolete_block )
         {
            _db.remove( *itr );
            itr = idx.begin();
         }
      }
   }
   else if ( actual_track_after_block <= note.block_num )
   {
      ilog( "Transaction status tracking activated at block ${block_num}, statuses will be available after block ${nominal_track_after_block}",
         ("block_num", note.block_num)("nominal_track_after_block", nominal_track_after_block) );
      tracking = true;
   }
}

/**
 * Retrieve the earliest transaction_id in a given range.
 *
 * \param[in] first_block_num The first block in the provided range
 * \param[in] last_block_num The last block in the provided range
 * \return The transaction_id of the earliest transaction that *should* be in our transaction status index
 */
fc::optional< transaction_id_type > transaction_status_impl::get_earliest_transaction_in_range( const uint32_t first_block_num, const uint32_t last_block_num )
{
   for ( uint32_t block_num = first_block_num; block_num <= last_block_num; block_num++ )
   {
      const auto block = _db.fetch_block_by_number( block_num );

      FC_ASSERT( block.valid(), "Could not read block ${n}", ("n", block_num) );

      if ( block->transactions.size() > 0 )
         return block->transactions.front().id();
   }

   return {};
}

/**
 * Retrieve the latest transaction_id in a given range.
 *
 * \param[in] first_block_num The first block in the provided range
 * \param[in] last_block_num The last block in the provided range
 * \return The transaction_id of the latest transaction that *should* be in our transaction status index
 */
fc::optional< transaction_id_type > transaction_status_impl::get_latest_transaction_in_range( const uint32_t first_block_num, const uint32_t last_block_num )
{
   for ( uint32_t block_num = last_block_num; block_num >= first_block_num; block_num-- )
   {
      const auto block = _db.fetch_block_by_number( block_num );

      FC_ASSERT( block.valid(), "Could not read block ${n}", ("n", block_num) );

      if ( block->transactions.size() > 0 )
         return block->transactions.back().id();
   }

   return {};
}

/**
 * Determine if the plugin state is valid.
 *
 * We determine validity by checking to see if we are aware of the first and last transactions
 * in the range we are supposed to be tracking.
 *
 * \return True if the transaction state is considered valid, otherwise false
 */
bool transaction_status_impl::state_is_valid()
{
   const auto head_block_num = _db.head_block_num();
   uint32_t earliest_tracked_block_num = get_earliest_tracked_block_num();

   const auto earliest_tx = get_earliest_transaction_in_range( earliest_tracked_block_num, head_block_num );
   const auto latest_tx = get_latest_transaction_in_range( earliest_tracked_block_num, head_block_num );

   bool lower_bound_is_valid = true;
   bool upper_bound_is_valid = true;

   if ( earliest_tx.valid() )
      lower_bound_is_valid = _db.find< transaction_status_object, by_trx_id >( *earliest_tx ) != nullptr;

   if ( latest_tx.valid() )
      upper_bound_is_valid = _db.find< transaction_status_object, by_trx_id >( *latest_tx ) != nullptr;
   
   return lower_bound_is_valid && upper_bound_is_valid;
}

/**
 * Rebuild the necessary transaction status index objects.
 *
 * If the transaction status plugin is enabled at some arbitrary point in time, it may not have
 * the necessary data to track transaction unless the state is rebuilt. We must clear out the
 * existing objects and recreate the objects as if it would have during run-time.
 */
void transaction_status_impl::rebuild_state()
{
   ilog( "Rebuilding transaction status state" );

   // Clear out the transaction status index
   const auto& tx_status_idx = _db.get_index< transaction_status_index >().indices().get< by_trx_id >();
   auto itr = tx_status_idx.begin();
   while( itr != tx_status_idx.end() )
   {
      _db.remove( *itr );
      itr = tx_status_idx.begin();
   }

   // Re-build the index from scratch
   const auto head_block_num = _db.head_block_num();
   uint32_t earliest_tracked_block_num = get_earliest_tracked_block_num();

   for ( uint32_t block_num = earliest_tracked_block_num; block_num <= head_block_num; block_num++ )
   {
      const auto block = _db.fetch_block_by_number( block_num );

      FC_ASSERT( block.valid(), "Could not read block ${n}", ("n", block_num) );

      for ( const auto& e : block->transactions )
         _db.create< transaction_status_object >( [&]( transaction_status_object& obj )
         {
            obj.transaction_id = e.id();
            obj.block_num = block_num;
         } );
   }
}

/**
 * Retrieve the earliest tracked block number.
 *
 * The earliest tracked block could be:
 *    1. The first block in the chain ( If the head block number minus the block depth is below 1, then we are tracking since the first block )
 *    2. One hour of blocks below the track after block user-defined variable plus 1 (actual track after block + 1)
 *    3. One hour of blocks below user-defined block depth plus 1 (head_block_number - actual block depth + 1)
 *
 * \return The earliest tracked block in transaction status state
 */
uint32_t transaction_status_impl::get_earliest_tracked_block_num()
{
   return std::max< int64_t >({ 1, int64_t( _db.head_block_num() ) - int64_t( actual_block_depth ) + 1, actual_track_after_block + 1 });
}

} // detail

transaction_status_plugin::transaction_status_plugin() {}
transaction_status_plugin::~transaction_status_plugin() {}

void transaction_status_plugin::set_program_options( boost::program_options::options_description& cli, boost::program_options::options_description& cfg )
{
   cfg.add_options()
      ( TRANSACTION_STATUS_BLOCK_DEPTH_KEY,   boost::program_options::value<uint32_t>()->default_value( TRANSACTION_STATUS_DEFAULT_BLOCK_DEPTH ), "Defines the number of blocks from the head block that transaction statuses will be tracked." )
      ( TRANSACTION_STATUS_TRACK_AFTER_KEY,   boost::program_options::value<uint32_t>()->default_value( TRANSACTION_STATUS_DEFAULT_TRACK_AFTER ), "Defines the block number the transaction status plugin will begin tracking." )
      ;
   cli.add_options()
      ( TRANSACTION_STATUS_REBUILD_STATE_KEY, boost::program_options::bool_switch()->default_value( false ), "Indicates that the transaction status plugin must re-build its state upon startup." )
      ;
}

void transaction_status_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   try
   {
      ilog( "transaction_status: plugin_initialize() begin" );
      my = std::make_unique< detail::transaction_status_impl >();

      fc::mutable_variant_object state_opts;

      if( options.count( TRANSACTION_STATUS_BLOCK_DEPTH_KEY ) )
      {
         my->nominal_block_depth = options.at( TRANSACTION_STATUS_BLOCK_DEPTH_KEY ).as< uint32_t >();
         state_opts[ TRANSACTION_STATUS_BLOCK_DEPTH_KEY ] = my->nominal_block_depth;
      }

      if( options.count( TRANSACTION_STATUS_TRACK_AFTER_KEY ) )
      {
         my->nominal_track_after_block = options.at( TRANSACTION_STATUS_TRACK_AFTER_KEY ).as< uint32_t >();
         state_opts[ TRANSACTION_STATUS_TRACK_AFTER_KEY ] = my->nominal_track_after_block;
      }

      if( options.count( TRANSACTION_STATUS_REBUILD_STATE_KEY ) )
         my->rebuild_state_flag = options.at( TRANSACTION_STATUS_REBUILD_STATE_KEY ).as< bool >();

      // We need to begin tracking 1 hour of blocks prior to the user provided track after block
      // A value of 0 indicates we should start tracking immediately
      my->actual_track_after_block = std::max< int64_t >( 0, int64_t( my->nominal_track_after_block ) - int64_t( STEEM_MAX_TIME_UNTIL_EXPIRATION / STEEM_BLOCK_INTERVAL ) );

      // We need to track 1 hour of blocks in addition to the depth the user would like us to track
      my->actual_block_depth = my->nominal_block_depth + ( STEEM_MAX_TIME_UNTIL_EXPIRATION / STEEM_BLOCK_INTERVAL );

      dlog( "transaction status initializing" );
      dlog( "  -> nominal block depth: ${block_depth}", ("block_depth", my->nominal_block_depth) );
      dlog( "  -> actual block depth: ${actual_block_depth}", ("actual_block_depth", my->actual_block_depth) );
      dlog( "  -> nominal track after block: ${track_after_block}", ("track_after_block", my->nominal_track_after_block) );
      dlog( "  -> actual track after block: ${actual_track_after_block}", ("actual_track_after_block", my->actual_track_after_block) );

      if ( !my->actual_track_after_block )
      {
         ilog( "Transaction status tracking activated" );
         my->tracking = true;
      }

      chain::add_plugin_index< transaction_status_index >( my->_db );

      appbase::app().get_plugin< chain::chain_plugin >().report_state_options( name(), state_opts );

      my->post_apply_transaction_connection = my->_db.add_post_apply_transaction_handler( [&]( const transaction_notification& note ) { try { my->on_post_apply_transaction( note ); } FC_LOG_AND_RETHROW() }, *this, 0 );
      my->post_apply_block_connection = my->_db.add_post_apply_block_handler( [&]( const block_notification& note ) { try { my->on_post_apply_block( note ); } FC_LOG_AND_RETHROW() }, *this, 0 );

      ilog( "transaction_status: plugin_initialize() end" );
   } FC_CAPTURE_AND_RETHROW()
}

void transaction_status_plugin::plugin_startup()
{
   try
   {
      ilog( "transaction_status: plugin_startup() begin" );
      if ( my->rebuild_state_flag )
      {
         my->rebuild_state();
      }
      else if ( !my->state_is_valid() )
      {
         wlog( "The transaction status plugin state does not contain tracking information for the last ${num_blocks} blocks. Re-run with the command line argument '--${rebuild_state_key}'.",
            ("num_blocks", my->nominal_block_depth) ("rebuild_state_key", TRANSACTION_STATUS_REBUILD_STATE_KEY) );
         exit( EXIT_FAILURE );
      }
      ilog( "transaction_status: plugin_startup() end" );
   } FC_CAPTURE_AND_RETHROW()
}

void transaction_status_plugin::plugin_shutdown()
{
   chain::util::disconnect_signal( my->post_apply_transaction_connection );
   chain::util::disconnect_signal( my->post_apply_block_connection );
}

uint32_t transaction_status_plugin::earliest_tracked_block_num()
{
   return my->get_earliest_tracked_block_num();
}

#ifdef IS_TEST_NET

bool transaction_status_plugin::state_is_valid()
{
   return my->state_is_valid();
}

void transaction_status_plugin::rebuild_state()
{
   my->rebuild_state();
}

#endif

} } } // steem::plugins::transaction_status

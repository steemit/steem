#include <steem/plugins/transaction_status/transaction_status_plugin.hpp>
#include <steem/plugins/transaction_status/transaction_status_objects.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/index.hpp>

#include <fc/io/json.hpp>

#define TRANSACTION_STATUS_BLOCK_DEPTH_KEY     "transaction-status-block-depth"
#define TRANSACTION_STATUS_DEFAULT_BLOCK_DEPTH  64000

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
   uint32_t                      block_depth;
   boost::signals2::connection   post_apply_transaction_connection;
   boost::signals2::connection   post_apply_block_connection;
};

void transaction_status_impl::on_post_apply_transaction( const transaction_notification& note )
{
   _db.create< transaction_status_object >( [&]( transaction_status_object& obj )
   {
      obj.transaction_id = note.transaction_id;
   } );
}

void transaction_status_impl::on_post_apply_block( const block_notification& note )
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
   if ( note.block_num > block_depth )
   {
      uint32_t earliest_tracked_block = note.block_num - block_depth;
      const auto& idx = _db.get_index< transaction_status_index >().indices().get< by_block_num >();
      const auto end = idx.upper_bound( earliest_tracked_block );
      std::for_each( idx.begin(), end, [&] ( const auto& e )
      {
         _db.remove( e );
      } );
   }
}

} // detail

transaction_status_plugin::transaction_status_plugin() {}
transaction_status_plugin::~transaction_status_plugin() {}

void transaction_status_plugin::set_program_options( boost::program_options::options_description& cli, boost::program_options::options_description& cfg )
{
   cfg.add_options()
      ( TRANSACTION_STATUS_BLOCK_DEPTH_KEY, boost::program_options::value<uint32_t>()->default_value( TRANSACTION_STATUS_DEFAULT_BLOCK_DEPTH ), "Defines the number of blocks from the head block that transaction statuses will be tracked." )
      ;
}

void transaction_status_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   try
   {
      ilog( "transaction_status: plugin_initialize() begin" );
      my = std::make_unique< detail::transaction_status_impl >();

      if( options.count( TRANSACTION_STATUS_BLOCK_DEPTH_KEY ) )
         my->block_depth = options.at( TRANSACTION_STATUS_BLOCK_DEPTH_KEY ).as< uint32_t >();

      chain::add_plugin_index< transaction_status_index >( my->_db );

      my->post_apply_transaction_connection = my->_db.add_post_apply_transaction_handler( [&]( const transaction_notification& note ) { try { my->on_post_apply_transaction( note ); } FC_LOG_AND_RETHROW() }, *this, 0 );
      my->post_apply_block_connection = my->_db.add_post_apply_block_handler( [&]( const block_notification& note ) { try { my->on_post_apply_block( note ); } FC_LOG_AND_RETHROW() }, *this, 0 );

      ilog( "transaction_status: plugin_initialize() end" );
   } FC_CAPTURE_AND_RETHROW()
}

void transaction_status_plugin::plugin_startup() {}

void transaction_status_plugin::plugin_shutdown()
{
   chain::util::disconnect_signal( my->post_apply_transaction_connection );
   chain::util::disconnect_signal( my->post_apply_block_connection );
}

uint32_t transaction_status_plugin::block_depth()
{
   return my->block_depth;
}

} } } // steem::plugins::transaction_status

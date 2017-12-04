#include <steem/plugins/block_log_info/block_log_info_plugin.hpp>
#include <steem/plugins/block_log_info/block_log_info_objects.hpp>

#include <steem/chain/account_object.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/index.hpp>
#include <steem/chain/operation_notification.hpp>

#include <graphene/schema/schema.hpp>
#include <graphene/schema/schema_impl.hpp>

namespace steem { namespace plugins { namespace block_log_info {

namespace detail {

class block_log_info_plugin_impl
{
   public:
      block_log_info_plugin_impl( block_log_info_plugin& _plugin ) :
         _db( appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db() ),
         _self( _plugin ) {}

      void on_applied_block( const signed_block& b );

      database&                     _db;
      block_log_info_plugin&        _self;
      boost::signals2::connection   on_applied_block_connection;
};

void block_log_info_plugin_impl::on_applied_block( const signed_block& b )
{
   if( b.block_num() == 1 )
   {
      _db.create< block_log_hash_state_object >( []( block_log_hash_state_object& bso )
      {
      } );
   }

   const block_log_hash_state_object& state = _db.get< block_log_hash_state_object, by_id >( block_log_hash_state_id_type(0) );
   uint64_t offset = state.total_size;
   std::vector< char > data = fc::raw::pack( b );
   for( int i=0; i<8; i++ )
   {
      data.push_back( (char) (offset & 0xFF) );
      offset >>= 8;
   }

   _db.modify( state, [&]( block_log_hash_state_object& bso )
   {
      bso.total_size += data.size();
      bso.rsha256.update( data.data(), data.size() );
   } );

   if( (b.block_num() % 100000) == 0 )
   {
      fc::restartable_sha256 h = state.rsha256;
      h.finish();
      ilog( "block_num=${b}   size=${ts}   hash=${h}",
         ("b", b.block_num())("ts", state.total_size)("h", h.hexdigest()) );
   }
}

} // detail

block_log_info_plugin::block_log_info_plugin() {}
block_log_info_plugin::~block_log_info_plugin() {}

void block_log_info_plugin::set_program_options( options_description& cli, options_description& cfg ){}

void block_log_info_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   my = std::make_unique< detail::block_log_info_plugin_impl >( *this );
   try
   {
      ilog( "Initializing block_log_info plugin" );
      chain::database& db = appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db();

      my->on_applied_block_connection = db.applied_block.connect( [&]( const signed_block& b ){ my->on_applied_block( b ); } );

      add_plugin_index< block_log_hash_state_index >(db);
   }
   FC_CAPTURE_AND_RETHROW()
}

void block_log_info_plugin::plugin_startup() {}

void block_log_info_plugin::plugin_shutdown()
{
   chain::util::disconnect_signal( my->on_applied_block_connection );
}

} } } // steem::plugins::block_log_info

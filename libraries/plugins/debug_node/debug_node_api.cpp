
#include <fc/filesystem.hpp>
#include <fc/optional.hpp>
#include <fc/variant_object.hpp>

#include <steemit/app/application.hpp>

#include <steemit/chain/block_database.hpp>
#include <steemit/chain/database.hpp>
#include <steemit/chain/witness_objects.hpp>

#include <graphene/utilities/key_conversion.hpp>

#include <steemit/plugins/debug_node/debug_node_api.hpp>
#include <steemit/plugins/debug_node/debug_node_plugin.hpp>

namespace steemit { namespace plugin { namespace debug_node {

namespace detail {

class debug_node_api_impl
{
   public:
      debug_node_api_impl( steemit::app::application& _app );

      void debug_push_blocks( const std::string& src_filename, uint32_t count );
      void debug_generate_blocks( const std::string& debug_key, uint32_t count );
      void debug_update_object( const fc::variant_object& update );
      //void debug_save_db( std::string db_path );
      void debug_stream_json_objects( const std::string& filename );
      void debug_stream_json_objects_flush();
      std::shared_ptr< steemit::plugin::debug_node::debug_node_plugin > get_plugin();

      steemit::app::application& app;
};

debug_node_api_impl::debug_node_api_impl( steemit::app::application& _app ) : app( _app )
{}


void debug_node_api_impl::debug_push_blocks( const std::string& src_filename, uint32_t count )
{
   if( count == 0 )
      return;

   std::shared_ptr< steemit::chain::database > db = app.chain_database();
   fc::path src_path = fc::path( src_filename );
   if( fc::is_directory( src_path ) )
   {
      ilog( "Loading ${n} from block_database ${fn}", ("n", count)("fn", src_filename) );
      steemit::chain::block_database bdb;
      bdb.open( src_path );
      uint32_t first_block = db->head_block_num()+1;
      for( uint32_t i=0; i<count; i++ )
      {
         fc::optional< steemit::chain::signed_block > block = bdb.fetch_by_number( first_block+i );
         if( !block.valid() )
         {
            wlog( "Block database ${fn} only contained ${i} of ${n} requested blocks", ("i", i)("n", count)("fn", src_filename) );
            return;
         }
         try
         {
            db->push_block( *block );
         }
         catch( const fc::exception& e )
         {
            elog( "Got exception pushing block ${bn} : ${bid} (${i} of ${n})", ("bn", block->block_num())("bid", block->id())("i", i)("n", count) );
            elog( "Exception backtrace: ${bt}", ("bt", e.to_detail_string()) );
         }
      }
      ilog( "Completed loading block_database successfully" );
      return;
   }
}

void debug_node_api_impl::debug_generate_blocks( const std::string& debug_key, uint32_t count )
{
   if( count == 0 )
      return;

   std::shared_ptr< debug_node_plugin > debug_plugin = get_plugin();

   fc::optional<fc::ecc::private_key> debug_private_key = graphene::utilities::wif_to_key( debug_key );
   FC_ASSERT( debug_private_key.valid() );
   steemit::chain::public_key_type debug_public_key = debug_private_key->get_public_key();

   std::shared_ptr< steemit::chain::database > db = app.chain_database();
   for( uint32_t i=0; i<count; i++ )
   {
      std::string scheduled_witness_name = db->get_scheduled_witness( 1 );
      fc::time_point_sec scheduled_time = db->get_slot_time( 1 );
      const chain::witness_object& scheduled_witness = db->get_witness( scheduled_witness_name );
      steemit::chain::public_key_type scheduled_key = scheduled_witness.signing_key;
      wlog( "scheduled key is: ${sk}   dbg key is: ${dk}", ("sk", scheduled_key)("dk", debug_public_key) );
      if( scheduled_key != debug_public_key )
      {
         wlog( "Modified key for witness ${w}", ("w", scheduled_witness_name) );
         fc::mutable_variant_object update;
         update("_action", "update")("id", scheduled_witness.id)("signing_key", debug_public_key);
         debug_plugin->debug_update( update );
      }
      db->generate_block( scheduled_time, scheduled_witness_name, *debug_private_key, steemit::chain::database::skip_nothing );
   }
}

void debug_node_api_impl::debug_update_object( const fc::variant_object& update )
{
   get_plugin()->debug_update( update );
}

std::shared_ptr< steemit::plugin::debug_node::debug_node_plugin > debug_node_api_impl::get_plugin()
{
   return app.get_plugin< debug_node_plugin >( "debug_node" );
}

void debug_node_api_impl::debug_stream_json_objects( const std::string& filename )
{
   get_plugin()->set_json_object_stream( filename );
}

void debug_node_api_impl::debug_stream_json_objects_flush()
{
   get_plugin()->flush_json_object_stream();
}

} // detail

debug_node_api::debug_node_api( steemit::app::application& app )
{
   my = std::make_shared< detail::debug_node_api_impl >(app);
}

void debug_node_api::on_api_startup() {}

void debug_node_api::debug_push_blocks( std::string source_filename, uint32_t count )
{
   my->debug_push_blocks( source_filename, count );
}

void debug_node_api::debug_generate_blocks( std::string debug_key, uint32_t count )
{
   my->debug_generate_blocks( debug_key, count );
}

void debug_node_api::debug_update_object( fc::variant_object update )
{
   my->debug_update_object( update );
}

void debug_node_api::debug_stream_json_objects( std::string filename )
{
   my->debug_stream_json_objects( filename );
}

void debug_node_api::debug_stream_json_objects_flush()
{
   my->debug_stream_json_objects_flush();
}

} } } // steemit::plugin::debug_node

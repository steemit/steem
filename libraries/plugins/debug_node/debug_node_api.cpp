
#include <fc/filesystem.hpp>
#include <fc/optional.hpp>
#include <fc/variant_object.hpp>

#include <steemit/app/api_context.hpp>
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

      uint32_t debug_push_blocks( const std::string& src_filename, uint32_t count );
      uint32_t debug_generate_blocks( const std::string& debug_key, uint32_t count );
      uint32_t debug_generate_blocks_until( const std::string& debug_key, const fc::time_point_sec& head_block_time, bool generate_sparsely );
      void debug_update_object( const fc::variant_object& update );
      //void debug_save_db( std::string db_path );
      void debug_stream_json_objects( const std::string& filename );
      void debug_stream_json_objects_flush();
      void debug_set_hardfork( uint32_t hardfork_id );
      std::shared_ptr< steemit::plugin::debug_node::debug_node_plugin > get_plugin();

      steemit::app::application& app;
};

debug_node_api_impl::debug_node_api_impl( steemit::app::application& _app ) : app( _app )
{}


uint32_t debug_node_api_impl::debug_push_blocks( const std::string& src_filename, uint32_t count )
{
   if( count == 0 )
      return 0;

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
            return i;
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
      return count;
   }
   return 0;
}

uint32_t debug_node_api_impl::debug_generate_blocks( const std::string& debug_key, uint32_t count )
{
   if( count == 0 )
      return 0;

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

   return count;
}

uint32_t debug_node_api_impl::debug_generate_blocks_until( const std::string& debug_key, const fc::time_point_sec& head_block_time, bool generate_sparsely )
{
   std::shared_ptr< steemit::chain::database > db = app.chain_database();

   if( db->head_block_time() >= head_block_time )
      return 0;

   uint32_t new_blocks = 0;

   if( generate_sparsely )
   {
      auto new_slot = db->get_slot_at_time( head_block_time );
      if( new_slot <= 1 )
         new_blocks += debug_generate_blocks( debug_key, 1 );
      else
      {
         fc::optional<fc::ecc::private_key> debug_private_key = graphene::utilities::wif_to_key( debug_key );
         FC_ASSERT( debug_private_key.valid() );
         steemit::chain::public_key_type debug_public_key = debug_private_key->get_public_key();

         auto scheduled_witness_name = db->get_scheduled_witness( new_slot );
         auto scheduled_time = db->get_slot_time( new_slot );
         const auto& scheduled_witness = db->get_witness( scheduled_witness_name );
         steemit::chain::public_key_type scheduled_key = scheduled_witness.signing_key;

         wlog( "scheduled key is: ${sk}   dbg key is: ${dk}", ("sk", scheduled_key)("dk", debug_public_key) );

         if( scheduled_key != debug_public_key )
         {
            wlog( "Modified key for witness ${w}", ("w", scheduled_witness_name) );
            fc::mutable_variant_object update;
            update("_action", "update")("id", scheduled_witness.id)("signing_key", debug_public_key);
            get_plugin()->debug_update( update );
         }

         db->generate_block( scheduled_time, scheduled_witness_name, *debug_private_key, steemit::chain::database::skip_nothing );
         new_blocks++;
      }
   }
   else
   {
      while( db->head_block_time() < head_block_time )
         new_blocks += debug_generate_blocks( debug_key, 1 );
   }

   return new_blocks;
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

void debug_node_api_impl::debug_set_hardfork( uint32_t hardfork_id )
{
   using namespace steemit::chain;

   if( hardfork_id > STEEMIT_NUM_HARDFORKS )
      return;

   fc::mutable_variant_object update;
   auto hfp_id = steemit::chain::hardfork_property_id_type();
   update( "_action", "set_hardfork" )( "id", hfp_id )( "hardfork_id", hardfork_id );
   get_plugin()->debug_update( update );
}

} // detail

debug_node_api::debug_node_api( const steemit::app::api_context& ctx )
{
   my = std::make_shared< detail::debug_node_api_impl >(ctx.app);
}

void debug_node_api::on_api_startup() {}

uint32_t debug_node_api::debug_push_blocks( std::string source_filename, uint32_t count )
{
   return my->debug_push_blocks( source_filename, count );
}

uint32_t debug_node_api::debug_generate_blocks( std::string debug_key, uint32_t count )
{
   return my->debug_generate_blocks( debug_key, count );
}

uint32_t debug_node_api::debug_generate_blocks_until( std::string debug_key, fc::time_point_sec head_block_time, bool generate_sparsely )
{
   return my->debug_generate_blocks_until( debug_key, head_block_time, generate_sparsely );
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

void debug_node_api::debug_set_hardfork( uint32_t hardfork_id )
{
   my->debug_set_hardfork( hardfork_id );
}

} } } // steemit::plugin::debug_node


#include <fc/filesystem.hpp>
#include <fc/optional.hpp>
#include <fc/variant_object.hpp>

#include <steemit/app/api_context.hpp>
#include <steemit/app/application.hpp>

#include <steemit/chain/protocol/block.hpp>
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

      uint32_t debug_push_blocks( const std::string& src_filename, uint32_t count, bool skip_validate_invariants );
      uint32_t debug_generate_blocks( const std::string& debug_key, uint32_t count );
      uint32_t debug_generate_blocks_until( const std::string& debug_key, const fc::time_point_sec& head_block_time, bool generate_sparsely );
      fc::optional< steemit::chain::signed_block > debug_pop_block();
      //void debug_push_block( const steemit::chain::signed_block& block );
      steemit::chain::witness_schedule_object debug_get_witness_schedule();
      steemit::chain::hardfork_property_object debug_get_hardfork_property_object();
      void debug_update_object( const fc::variant_object& update );
      fc::variant_object debug_get_edits();
      void debug_set_edits( const fc::variant_object& edits );
      //void debug_save_db( std::string db_path );
      void debug_stream_json_objects( const std::string& filename );
      void debug_stream_json_objects_flush();
      void debug_set_hardfork( uint32_t hardfork_id );
      bool debug_has_hardfork( uint32_t hardfork_id );
      std::shared_ptr< steemit::plugin::debug_node::debug_node_plugin > get_plugin();

      steemit::app::application& app;
};

debug_node_api_impl::debug_node_api_impl( steemit::app::application& _app ) : app( _app )
{}


uint32_t debug_node_api_impl::debug_push_blocks( const std::string& src_filename, uint32_t count, bool skip_validate_invariants )
{
   if( count == 0 )
      return 0;

   std::shared_ptr< steemit::chain::database > db = app.chain_database();
   fc::path src_path = fc::path( src_filename );
   if( fc::is_directory( src_path ) )
   {
      ilog( "Loading ${n} from block_database ${fn}", ("n", count)("fn", src_filename) );
      idump( (src_filename)(count)(skip_validate_invariants) );
      steemit::chain::block_database bdb;
      bdb.open( src_path );
      uint32_t first_block = db->head_block_num()+1;
      uint32_t skip_flags = steemit::chain::database::skip_nothing;
      if( skip_validate_invariants )
         skip_flags = skip_flags | steemit::chain::database::skip_validate_invariants;
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
            db->push_block( *block, skip_flags );
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
   return get_plugin()->debug_generate_blocks( debug_key, count );
}

uint32_t debug_node_api_impl::debug_generate_blocks_until( const std::string& debug_key, const fc::time_point_sec& head_block_time, bool generate_sparsely )
{
   return get_plugin()->debug_generate_blocks_until( debug_key, head_block_time, generate_sparsely );
}

fc::optional< steemit::chain::signed_block > debug_node_api_impl::debug_pop_block()
{
   std::shared_ptr< steemit::chain::database > db = app.chain_database();
   return db->fetch_block_by_number( db->head_block_num() );
}

/*void debug_node_api_impl::debug_push_block( const steemit::chain::signed_block& block )
{
   app.chain_database()->push_block( block );
}*/

steemit::chain::witness_schedule_object debug_node_api_impl::debug_get_witness_schedule()
{
   return steemit::chain::witness_schedule_id_type()( *app.chain_database() );
}

steemit::chain::hardfork_property_object debug_node_api_impl::debug_get_hardfork_property_object()
{
   return steemit::chain::hardfork_property_id_type()( *app.chain_database() );
}

void debug_node_api_impl::debug_update_object( const fc::variant_object& update )
{
   get_plugin()->debug_update( update );
}

fc::variant_object debug_node_api_impl::debug_get_edits()
{
   fc::mutable_variant_object result;
   get_plugin()->save_debug_updates( result );
   return fc::variant_object( std::move( result ) );
}

void debug_node_api_impl::debug_set_edits( const fc::variant_object& edits )
{
   get_plugin()->load_debug_updates( edits );
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

bool debug_node_api_impl::debug_has_hardfork( uint32_t hardfork_id )
{
   idump( (steemit::chain::hardfork_property_id_type()( *app.chain_database() ))(hardfork_id) );
   return steemit::chain::hardfork_property_id_type()( *app.chain_database() ).last_hardfork >= hardfork_id;
}

} // detail

debug_node_api::debug_node_api( const steemit::app::api_context& ctx )
{
   my = std::make_shared< detail::debug_node_api_impl >(ctx.app);
}

void debug_node_api::on_api_startup() {}

uint32_t debug_node_api::debug_push_blocks( std::string source_filename, uint32_t count, bool skip_validate_invariants )
{
   return my->debug_push_blocks( source_filename, count, skip_validate_invariants );
}

uint32_t debug_node_api::debug_generate_blocks( std::string debug_key, uint32_t count )
{
   return my->debug_generate_blocks( debug_key, count );
}

uint32_t debug_node_api::debug_generate_blocks_until( std::string debug_key, fc::time_point_sec head_block_time, bool generate_sparsely )
{
   return my->debug_generate_blocks_until( debug_key, head_block_time, generate_sparsely );
}

fc::optional< steemit::chain::signed_block > debug_node_api::debug_pop_block()
{
   return my->debug_pop_block();
}

/*void debug_node_api::debug_push_block( steemit::chain::signed_block& block )
{
   my->debug_push_block( block );
}*/

steemit::chain::witness_schedule_object debug_node_api::debug_get_witness_schedule()
{
   return my->debug_get_witness_schedule();
}

steemit::chain::hardfork_property_object debug_node_api::debug_get_hardfork_property_object()
{
   return my->debug_get_hardfork_property_object();
}

void debug_node_api::debug_update_object( fc::variant_object update )
{
   my->debug_update_object( update );
}

fc::variant_object debug_node_api::debug_get_edits()
{
   return my->debug_get_edits();
}

void debug_node_api::debug_set_edits( fc::variant_object edits )
{
   my->debug_set_edits(edits);
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

bool debug_node_api::debug_has_hardfork( uint32_t hardfork_id )
{
   return my->debug_has_hardfork( hardfork_id );
}

} } } // steemit::plugin::debug_node

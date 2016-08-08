
#include <steemit/app/application.hpp>
#include <steemit/app/plugin.hpp>
#include <steemit/plugins/debug_node/debug_node_api.hpp>
#include <steemit/plugins/debug_node/debug_node_plugin.hpp>

#include <fc/io/json.hpp>

#include <graphene/utilities/key_conversion.hpp>

#include <sstream>
#include <string>

namespace steemit { namespace plugin { namespace debug_node {

debug_node_plugin::debug_node_plugin() {}
debug_node_plugin::~debug_node_plugin() {}

std::string debug_node_plugin::plugin_name()const
{
   return "debug_node";
}

void debug_node_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
}

void debug_node_plugin::plugin_startup()
{
   ilog("debug_node_plugin::plugin_startup() begin");
   chain::database& db = database();

   // connect needed signals

   _applied_block_conn  = db.applied_block.connect([this](const chain::signed_block& b){ on_applied_block(b); });
   _changed_objects_conn = db.changed_objects.connect([this](const std::vector<graphene::db::object_id_type>& ids){ on_changed_objects(ids); });
   _removed_objects_conn = db.removed_objects.connect([this](const std::vector<const graphene::db::object*>& objs){ on_removed_objects(objs); });

   app().register_api_factory< debug_node_api >( "debug_node_api" );
}

void debug_apply_update( chain::database& db, const fc::variant_object& vo )
{
   static const uint8_t
      db_action_nil = 0,
      db_action_create = 1,
      db_action_write = 2,
      db_action_update = 3,
      db_action_delete = 4,
      db_action_set_hardfork = 5;

   wlog( "debug_apply_update:  ${o}", ("o", vo) );

   // "_action" : "create"   object must not exist, unspecified fields take defaults
   // "_action" : "write"    object may exist, is replaced entirely, unspecified fields take defaults
   // "_action" : "update"   object must exist, unspecified fields don't change
   // "_action" : "delete"   object must exist, will be deleted

   // if _action is unspecified:
   // - delete if object contains only ID field
   // - otherwise, write

   graphene::db::object_id_type oid;
   uint8_t action = db_action_nil;
   auto it_id = vo.find("id");
   FC_ASSERT( it_id != vo.end() );

   from_variant( it_id->value(), oid );
   action = ( vo.size() == 1 ) ? db_action_delete : db_action_write;

   from_variant( vo["id"], oid );
   if( vo.size() == 1 )
      action = db_action_delete;
   auto it_action = vo.find("_action" );
   if( it_action != vo.end() )
   {
      const std::string& str_action = it_action->value().get_string();
      if( str_action == "create" )
         action = db_action_create;
      else if( str_action == "write" )
         action = db_action_write;
      else if( str_action == "update" )
         action = db_action_update;
      else if( str_action == "delete" )
         action = db_action_delete;
      else if( str_action == "set_hardfork" )
         action = db_action_set_hardfork;
   }

   auto& idx = db.get_index( oid );

   switch( action )
   {
      case db_action_create:
         /*
         idx.create( [&]( object& obj )
         {
            idx.object_from_variant( vo, obj );
         } );
         */
         FC_ASSERT( false );
         break;
      case db_action_write:
         db.modify( db.get_object( oid ), [&]( graphene::db::object& obj )
         {
            idx.object_default( obj );
            idx.object_from_variant( vo, obj );
         } );
         break;
      case db_action_update:
         db.modify( db.get_object( oid ), [&]( graphene::db::object& obj )
         {
            idx.object_from_variant( vo, obj );
         } );
         break;
      case db_action_delete:
         db.remove( db.get_object( oid ) );
         break;
      case db_action_set_hardfork:
         {
            uint32_t hardfork_id;
            from_variant( vo[ "hardfork_id" ], hardfork_id );
            db.set_hardfork( hardfork_id, false );
         }
         break;
      default:
         FC_ASSERT( false );
   }
}

uint32_t debug_node_plugin::debug_generate_blocks( const std::string& debug_key, uint32_t count )
{
   if( count == 0 )
      return 0;

   fc::optional<fc::ecc::private_key> debug_private_key = graphene::utilities::wif_to_key( debug_key );
   FC_ASSERT( debug_private_key.valid() );
   chain::public_key_type debug_public_key = debug_private_key->get_public_key();

   chain::database& db = database();
   for( uint32_t i=0; i<count; i++ )
   {
      std::string scheduled_witness_name = db.get_scheduled_witness( 1 );
      fc::time_point_sec scheduled_time = db.get_slot_time( 1 );
      const chain::witness_object& scheduled_witness = db.get_witness( scheduled_witness_name );
      steemit::chain::public_key_type scheduled_key = scheduled_witness.signing_key;
      wlog( "scheduled key is: ${sk}   dbg key is: ${dk}", ("sk", scheduled_key)("dk", debug_public_key) );
      if( scheduled_key != debug_public_key )
      {
         wlog( "Modified key for witness ${w}", ("w", scheduled_witness_name) );
         fc::mutable_variant_object update;
         update("_action", "update")("id", scheduled_witness.id)("signing_key", debug_public_key);
         debug_update( update );
      }
      db.generate_block( scheduled_time, scheduled_witness_name, *debug_private_key, steemit::chain::database::skip_nothing );
   }

   return count;
}

uint32_t debug_node_plugin::debug_generate_blocks_until( const std::string& debug_key, const fc::time_point_sec& head_block_time, bool generate_sparsely )
{
  chain::database& db = database();

   if( db.head_block_time() >= head_block_time )
      return 0;

   uint32_t new_blocks = 0;

   if( generate_sparsely )
   {
      auto new_slot = db.get_slot_at_time( head_block_time );

      if( new_slot == 0 )
         return 0;

      fc::optional<fc::ecc::private_key> debug_private_key = graphene::utilities::wif_to_key( debug_key );
      FC_ASSERT( debug_private_key.valid() );
      steemit::chain::public_key_type debug_public_key = debug_private_key->get_public_key();

      std::string scheduled_witness_name = db.get_scheduled_witness( new_slot );
      fc::time_point_sec scheduled_time = db.get_slot_time( new_slot );
      const chain::witness_object& scheduled_witness = db.get_witness( scheduled_witness_name );
      steemit::chain::public_key_type scheduled_key = scheduled_witness.signing_key;

      wlog( "scheduled key is: ${sk}   dbg key is: ${dk}", ("sk", scheduled_key)("dk", debug_public_key) );

      if( scheduled_key != debug_public_key )
      {
         wlog( "Modified key for witness ${w}", ("w", scheduled_witness_name) );
         fc::mutable_variant_object update;
         update("_action", "update")("id", scheduled_witness.id)("signing_key", debug_public_key);
         debug_update( update );
      }

      db.generate_block( scheduled_time, scheduled_witness_name, *debug_private_key, steemit::chain::database::skip_nothing );
      new_blocks++;

      FC_ASSERT( head_block_time.sec_since_epoch() - db.head_block_time().sec_since_epoch() < STEEMIT_BLOCK_INTERVAL, "", ("desired_time", head_block_time)("db.head_block_time()",db.head_block_time()) );
   }
   else
   {
      while( db.head_block_time() < head_block_time )
         new_blocks += debug_generate_blocks( debug_key, 1 );
   }

   return new_blocks;
}

void debug_node_plugin::apply_debug_updates()
{
   // this was a method on database in Graphene
   chain::database& db = database();
   chain::block_id_type head_id = db.head_block_id();
   auto it = _debug_updates.find( head_id );
   if( it == _debug_updates.end() )
      return;
   for( const fc::variant_object& update : it->second )
      debug_apply_update( db, update );
}

void debug_node_plugin::debug_update( const fc::variant_object& update )
{
   // this was a method on database in Graphene
   chain::database& db = database();
   chain::block_id_type head_id = db.head_block_id();
   auto it = _debug_updates.find( head_id );
   if( it == _debug_updates.end() )
      it = _debug_updates.emplace( head_id, std::vector< fc::variant_object >() ).first;
   it->second.emplace_back( update );

   fc::optional<chain::signed_block> head_block = db.fetch_block_by_id( head_id );
   FC_ASSERT( head_block.valid() );

   // What the last block does has been changed by adding to node_property_object, so we have to re-apply it
   db.pop_block();
   db.push_block( *head_block );
}

void debug_node_plugin::on_changed_objects( const std::vector<graphene::db::object_id_type>& ids )
{
   // this was a method on database in Graphene
   if( _json_object_stream && (ids.size() > 0) )
   {
      const chain::database& db = database();
      for( const graphene::db::object_id_type& oid : ids )
      {
         const graphene::db::object* obj = db.find_object( oid );
         if( obj == nullptr )
         {
            (*_json_object_stream) << "{\"id\":" << fc::json::to_string( oid ) << "}\n";
         }
         else
         {
            (*_json_object_stream) << fc::json::to_string( obj->to_variant() ) << '\n';
         }
      }
   }
}

void debug_node_plugin::on_removed_objects( const std::vector<const graphene::db::object*> objs )
{
   /*
   if( _json_object_stream )
   {
      for( const graphene::db::object* obj : objs )
      {
         (*_json_object_stream) << "{\"id\":" << fc::json::to_string( obj->id ) << "}\n";
      }
   }
   */
}

void debug_node_plugin::on_applied_block( const chain::signed_block& b )
{
   if( !_debug_updates.empty() )
      apply_debug_updates();

   if( _json_object_stream )
   {
      (*_json_object_stream) << "{\"bn\":" << fc::to_string( b.block_num() ) << "}\n";
   }
}

void debug_node_plugin::set_json_object_stream( const std::string& filename )
{
   if( _json_object_stream )
   {
      _json_object_stream->close();
      _json_object_stream.reset();
   }
   _json_object_stream = std::make_shared< std::ofstream >( filename );
}

void debug_node_plugin::flush_json_object_stream()
{
   if( _json_object_stream )
      _json_object_stream->flush();
}

void debug_node_plugin::plugin_shutdown()
{
   if( _json_object_stream )
   {
      _json_object_stream->close();
      _json_object_stream.reset();
   }
   return;
}

} } }

STEEMIT_DEFINE_PLUGIN( debug_node, steemit::plugin::debug_node::debug_node_plugin )

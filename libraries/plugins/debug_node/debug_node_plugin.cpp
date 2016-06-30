
#include <steemit/app/application.hpp>
#include <steemit/app/plugin.hpp>
#include <steemit/plugins/debug_node/debug_node_api.hpp>
#include <steemit/plugins/debug_node/debug_node_plugin.hpp>

#include <fc/io/buffered_iostream.hpp>
#include <fc/io/fstream.hpp>
#include <fc/io/json.hpp>

#include <sstream>
#include <string>

namespace steemit { namespace plugin { namespace debug_node {

debug_node_plugin::debug_node_plugin() {}
debug_node_plugin::~debug_node_plugin() {}

std::string debug_node_plugin::plugin_name()const
{
   return "debug_node";
}

void debug_node_plugin::plugin_set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg )
{
   cli.add_options()
      ("edit-script,e", boost::program_options::value< std::vector< std::string > >()->composing(), "Database edits to apply on startup (may specify multiple times)");
   cfg.add(cli);
}

void debug_node_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   if( options.count("edit-script") == 0 )
      return;
   _edit_scripts = options.at("edit-script").as< std::vector< std::string > >();
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

   for( const std::string& fn : _edit_scripts )
   {
      std::shared_ptr< fc::ifstream > stream = std::make_shared< fc::ifstream >( fc::path(fn) );
      fc::buffered_istream bstream(stream);
      fc::variant v = fc::json::from_stream( bstream, fc::json::strict_parser );
      load_debug_updates( v.get_object() );
   }
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

void debug_node_plugin::save_debug_updates( fc::mutable_variant_object& target )
{
   for( const std::pair< chain::block_id_type, std::vector< fc::variant_object > >& update : _debug_updates )
   {
      fc::variant v;
      fc::to_variant( update.second, v );
      target.set( update.first.str(), v );
   }
}

void debug_node_plugin::load_debug_updates( const fc::variant_object& target )
{
   for( auto it=target.begin(); it != target.end(); ++it)
   {
      std::vector< fc::variant_object > o;
      fc::from_variant(it->value(), o);
      _debug_updates[ chain::block_id_type( it->key() ) ] = o;
   }
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

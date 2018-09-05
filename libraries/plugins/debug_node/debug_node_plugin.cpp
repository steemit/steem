#include <steem/plugins/debug_node/debug_node_plugin.hpp>

#include <steem/plugins/witness/block_producer.hpp>
#include <steem/chain/witness_objects.hpp>

#include <fc/io/buffered_iostream.hpp>
#include <fc/io/fstream.hpp>
#include <fc/io/json.hpp>

#include <fc/thread/future.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/thread/scoped_lock.hpp>

#include <steem/utilities/key_conversion.hpp>

#include <sstream>
#include <string>

namespace steem { namespace plugins { namespace debug_node {

namespace detail {
class debug_node_plugin_impl
{
   public:
      debug_node_plugin_impl();
      virtual ~debug_node_plugin_impl();

      chain::database&                          _db;
      boost::signals2::connection               _post_apply_block_conn;
};

debug_node_plugin_impl::debug_node_plugin_impl() :
   _db( appbase::app().get_plugin< chain::chain_plugin >().db() ) {}
debug_node_plugin_impl::~debug_node_plugin_impl() {}

}

debug_node_plugin::debug_node_plugin() {}
debug_node_plugin::~debug_node_plugin() {}

void debug_node_plugin::set_program_options(
   options_description& cli,
   options_description& cfg )
{
   cfg.add_options()
      ("debug-node-edit-script,e",
         boost::program_options::value< std::vector< std::string > >()->composing(),
            "Database edits to apply on startup (may specify multiple times)")
      ("edit-script", boost::program_options::value< std::vector< std::string > >()->composing(),
          "Database edits to apply on startup (may specify multiple times). Deprecated in favor of debug-node-edit-script.")
      ;
}

void debug_node_plugin::plugin_initialize( const variables_map& options )
{
   my = std::make_shared< detail::debug_node_plugin_impl >();

   if( options.count( "debug-node-edit-script" ) > 0 )
   {
      _edit_scripts = options.at( "debug-node-edit-script" ).as< std::vector< std::string > >();
   }

   if( options.count("edit-script") > 0 )
   {
      wlog( "edit-scripts is deprecated in favor of debug-node-edit-script" );
      auto scripts = options.at( "edit-script" ).as< std::vector< std::string > >();
      _edit_scripts.insert( _edit_scripts.end(), scripts.begin(), scripts.end() );
   }

   // connect needed signals
   my->_post_apply_block_conn = my->_db.add_post_apply_block_handler(
      [this](const chain::block_notification& note){ on_post_apply_block(note); }, *this, 0 );
}

void debug_node_plugin::plugin_startup()
{
   /*for( const std::string& fn : _edit_scripts )
   {
      std::shared_ptr< fc::ifstream > stream = std::make_shared< fc::ifstream >( fc::path(fn) );
      fc::buffered_istream bstream(stream);
      fc::variant v = fc::json::from_stream( bstream, fc::json::strict_parser );
      load_debug_updates( v.get_object() );
   }*/
}

chain::database& debug_node_plugin::database() { return my->_db; }

/*
void debug_apply_update( chain::database& db, const fc::variant_object& vo, bool logging )
{
   static const uint8_t
      db_action_nil = 0,
      db_action_create = 1,
      db_action_write = 2,
      db_action_update = 3,
      db_action_delete = 4,
      db_action_set_hardfork = 5;
   if( logging ) wlog( "debug_apply_update:  ${o}", ("o", vo) );

   // "_action" : "create"   object must not exist, unspecified fields take defaults
   // "_action" : "write"    object may exist, is replaced entirely, unspecified fields take defaults
   // "_action" : "update"   object must exist, unspecified fields don't change
   // "_action" : "delete"   object must exist, will be deleted

   // if _action is unspecified:
   // - delete if object contains only ID field
   // - otherwise, write

   graphene::db2::generic_id oid;
   uint8_t action = db_action_nil;
   auto it_id = vo.find("id");
   FC_ASSERT( it_id != vo.end() );

   from_variant( it_id->value(), oid );
   action = ( vo.size() == 1 ) ? db_action_delete : db_action_write;

   from_variant( vo["id"], oid );
   if( vo.size() == 1 )
      action = db_action_delete;

   fc::mutable_variant_object mvo( vo );
   mvo( "id", oid._id );
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

   switch( action )
   {
      case db_action_create:

         idx.create( [&]( object& obj )
         {
            idx.object_from_variant( vo, obj );
         } );

         FC_ASSERT( false );
         break;
      case db_action_write:
         db.modify( db.get_object( oid ), [&]( graphene::db::object& obj )
         {
            idx.object_default( obj );
            idx.object_from_variant( vo, obj );
         } );
         FC_ASSERT( false );
         break;
      case db_action_update:
         db.modify_variant( oid, mvo );
         break;
      case db_action_delete:
         db.remove_object( oid );
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
*/

uint32_t debug_node_plugin::debug_generate_blocks(
   const std::string& debug_key,
   uint32_t count,
   uint32_t skip,
   uint32_t miss_blocks
)
{
   debug_generate_blocks_args args;
   debug_generate_blocks_return ret;

   args.debug_key = debug_key;
   args.count = count;
   args.skip = skip;
   args.miss_blocks = miss_blocks;
   debug_generate_blocks( ret, args );
   return ret.blocks;
}

void debug_node_plugin::debug_generate_blocks(
   debug_generate_blocks_return& ret,
   const debug_generate_blocks_args& args )
{
   if( args.count == 0 )
   {
      ret.blocks = 0;
      return;
   }

   fc::optional<fc::ecc::private_key> debug_private_key;
   chain::public_key_type debug_public_key;
   if( args.debug_key != "" )
   {
      debug_private_key = steem::utilities::wif_to_key( args.debug_key );
      FC_ASSERT( debug_private_key.valid() );
      debug_public_key = debug_private_key->get_public_key();
   }
   else
   {
      if( logging ) elog( "Skipping generation because I don't know the private key");
      ret.blocks = 0;
      return;
   }

   chain::database& db = database();
   witness::block_producer bp( db );
   uint32_t slot = args.miss_blocks+1, produced = 0;
   while( produced < args.count )
   {
      uint32_t new_slot = args.miss_blocks+1;
      std::string scheduled_witness_name = db.get_scheduled_witness( slot );
      fc::time_point_sec scheduled_time = db.get_slot_time( slot );
      const chain::witness_object& scheduled_witness = db.get_witness( scheduled_witness_name );
      chain::public_key_type scheduled_key = scheduled_witness.signing_key;
      if( logging )
      {
         wlog( "slot: ${sl}   time: ${t}   scheduled key is: ${sk}   dbg key is: ${dk}",
            ("sk", scheduled_key)("dk", debug_public_key)("sl", slot)("t", scheduled_time) );
      }
      if( scheduled_key != debug_public_key )
      {
         if( args.edit_if_needed )
         {
            if( logging ) wlog( "Modified key for witness ${w}", ("w", scheduled_witness_name) );
            debug_update( [=]( chain::database& db )
            {
               db.modify( db.get_witness( scheduled_witness_name ), [&]( chain::witness_object& w )
               {
                  w.signing_key = debug_public_key;
               });
            }, args.skip );
         }
         else
            break;
      }

      bp.generate_block( scheduled_time, scheduled_witness_name, *debug_private_key, args.skip );
      ++produced;
      slot = new_slot;
   }

   ret.blocks = produced;
   return;
}

uint32_t debug_node_plugin::debug_generate_blocks_until(
   const std::string& debug_key,
   const fc::time_point_sec& head_block_time,
   bool generate_sparsely,
   uint32_t skip
)
{
   chain::database& db = database();

   if( db.head_block_time() >= head_block_time )
      return 0;

   uint32_t new_blocks = 0;

   if( generate_sparsely )
   {
      new_blocks += debug_generate_blocks( debug_key, 1, skip );
      auto slots_to_miss = db.get_slot_at_time( head_block_time );
      if( slots_to_miss > 1 )
      {
         slots_to_miss--;
         new_blocks += debug_generate_blocks( debug_key, 1, skip, slots_to_miss );
      }
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
   //for( const fc::variant_object& update : it->second )
   //   debug_apply_update( db, update, logging );
   for( auto& update : it->second )
      update( db );
}

void debug_node_plugin::on_post_apply_block( const chain::block_notification& note )
{
   try
   {
   if( !_debug_updates.empty() )
      apply_debug_updates();
   }
   FC_LOG_AND_RETHROW()
}

/*void debug_node_plugin::set_json_object_stream( const std::string& filename )
{
   if( _json_object_stream )
   {
      _json_object_stream->close();
      _json_object_stream.reset();
   }
   _json_object_stream = std::make_shared< std::ofstream >( filename );
}*/

/*void debug_node_plugin::flush_json_object_stream()
{
   if( _json_object_stream )
      _json_object_stream->flush();
}*/

/*void debug_node_plugin::save_debug_updates( fc::mutable_variant_object& target )
{
   for( const std::pair< chain::block_id_type, std::vector< fc::variant_object > >& update : _debug_updates )
   {
      fc::variant v;
      fc::to_variant( update.second, v );
      target.set( update.first.str(), v );
   }
}*/

/*void debug_node_plugin::load_debug_updates( const fc::variant_object& target )
{
   for( auto it=target.begin(); it != target.end(); ++it)
   {
      std::vector< fc::variant_object > o;
      fc::from_variant(it->value(), o);
      _debug_updates[ chain::block_id_type( it->key() ) ] = o;
   }
}*/

void debug_node_plugin::plugin_shutdown()
{
   chain::util::disconnect_signal( my->_post_apply_block_conn );
   /*if( _json_object_stream )
   {
      _json_object_stream->close();
      _json_object_stream.reset();
   }*/
   return;
}

} } } // steem::plugins::debug_node

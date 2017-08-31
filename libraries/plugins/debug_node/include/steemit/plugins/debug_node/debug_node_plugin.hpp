
#pragma once
#include <steemit/plugins/chain/chain_plugin.hpp>

#include <fc/variant_object.hpp>

#include <map>
#include <fstream>

#define STEEM_DEBUG_NODE_PLUGIN_NAME "debug_node"

namespace steemit { namespace protocol {
   struct chain_properties;
   struct pow2;
   struct signed_block;
} }

namespace steemit { namespace plugins { namespace debug_node {

using namespace appbase;

namespace detail { class debug_node_plugin_impl; }

class debug_node_plugin : public plugin< debug_node_plugin >
{
   public:
      debug_node_plugin();
      virtual ~debug_node_plugin();

      APPBASE_PLUGIN_REQUIRES( (chain::chain_plugin) )

      static const std::string& name() { static std::string name = STEEM_DEBUG_NODE_PLUGIN_NAME; return name; }

      virtual void set_program_options(
         options_description& cli,
         options_description& cfg ) override;
      void plugin_initialize( const variables_map& options );
      void plugin_startup();
      void plugin_shutdown();

      chain::database& database();

      template< typename Lambda >
      void debug_update( Lambda&& callback, uint32_t skip = steemit::chain::database::skip_nothing )
      {
         // this was a method on database in Graphene
         chain::database& db = database();
         chain::block_id_type head_id = db.head_block_id();
         auto it = _debug_updates.find( head_id );
         if( it == _debug_updates.end() )
            it = _debug_updates.emplace( head_id, std::vector< std::function< void( chain::database& ) > >() ).first;
         it->second.emplace_back( callback );

         fc::optional<chain::signed_block> head_block = db.fetch_block_by_id( head_id );
         FC_ASSERT( head_block.valid() );

         // What the last block does has been changed by adding to node_property_object, so we have to re-apply it
         db.pop_block();
         db.push_block( *head_block, skip );
      }


      uint32_t debug_generate_blocks(
         const std::string& debug_key,
         uint32_t count,
         uint32_t skip = steemit::chain::database::skip_nothing,
         uint32_t miss_blocks = 0
         );
      uint32_t debug_generate_blocks_until(
         const std::string& debug_key,
         const fc::time_point_sec& head_block_time,
         bool generate_sparsely,
         uint32_t skip = steemit::chain::database::skip_nothing
         );

      void set_json_object_stream( const std::string& filename );
      void flush_json_object_stream();

      void save_debug_updates( fc::mutable_variant_object& target );
      void load_debug_updates( const fc::variant_object& target );

      bool logging = true;

   private:
      void on_applied_block( const protocol::signed_block& b );

      void apply_debug_updates();

      std::map<protocol::public_key_type, fc::ecc::private_key> _private_keys;

      std::shared_ptr< detail::debug_node_plugin_impl > _my;

      //std::shared_ptr< std::ofstream > _json_object_stream;
      boost::signals2::scoped_connection _applied_block_conn;
      boost::signals2::scoped_connection _changed_objects_conn;
      boost::signals2::scoped_connection _removed_objects_conn;

      std::vector< std::string > _edit_scripts;
      //std::map< protocol::block_id_type, std::vector< fc::variant_object > > _debug_updates;
      std::map< protocol::block_id_type, std::vector< std::function< void( chain::database& ) > > > _debug_updates;
};

} } }

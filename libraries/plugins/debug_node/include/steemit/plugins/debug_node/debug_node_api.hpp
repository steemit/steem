
#pragma once

#include <memory>
#include <string>

#include <fc/api.hpp>
#include <fc/optional.hpp>
#include <fc/variant_object.hpp>

#include <steemit/chain/protocol/block.hpp>
#include <steemit/chain/witness_objects.hpp>

namespace steemit { namespace app {
   struct api_context;
} }

namespace steemit { namespace plugin { namespace debug_node {

namespace detail {
class debug_node_api_impl;
}

class debug_node_api
{
   public:
      debug_node_api( const steemit::app::api_context& ctx );

      void on_api_startup();

      /**
       * Push blocks from existing database.
       */
      uint32_t debug_push_blocks( std::string src_filename, uint32_t count, bool skip_validate_invariants = false );

      /**
       * Generate blocks locally.
       */
      uint32_t debug_generate_blocks( std::string debug_key, uint32_t count );

      /*
       * Generate blocks locally until a specified head block time. Can generate them sparsely.
       */
      uint32_t debug_generate_blocks_until( std::string debug_key, fc::time_point_sec head_block_time, bool generate_sparsely = true );

      /*
       * Pop a block from the blockchain, returning it
       */
      fc::optional< steemit::chain::signed_block > debug_pop_block();

      /*
       * Push an already constructed block onto the blockchain. For use with pop_block to traverse state block by block.
       */
      // not implemented
      //void debug_push_block( steemit::chain::signed_block& block );

      steemit::chain::witness_schedule_object debug_get_witness_schedule();

      steemit::chain::hardfork_property_object debug_get_hardfork_property_object();

      /**
       * Directly manipulate database objects (will undo and re-apply last block with new changes post-applied).
       */
      void debug_update_object( fc::variant_object update );

      /**
       * Start a node with given initial path.
       */
      // not implemented
      //void start_node( std::string name, std::string initial_db_path );

      /**
       * Save the database to disk.
       */
      // not implemented
      //void save_db( std::string db_path );

      /**
       * Stream objects to file.  (Hint:  Create with mkfifo and pipe it to a script)
       */

      void debug_stream_json_objects( std::string filename );

      /**
       * Flush streaming file.
       */
      void debug_stream_json_objects_flush();

      void debug_set_hardfork( uint32_t hardfork_id );

      bool debug_has_hardfork( uint32_t hardfork_id );

      std::shared_ptr< detail::debug_node_api_impl > my;
};

} } }

FC_API(steemit::plugin::debug_node::debug_node_api,
       (debug_push_blocks)
       (debug_generate_blocks)
       (debug_generate_blocks_until)
       (debug_pop_block)
       //(debug_push_block)
       (debug_update_object)
       (debug_stream_json_objects)
       (debug_stream_json_objects_flush)
       (debug_set_hardfork)
       (debug_has_hardfork)
       (debug_get_witness_schedule)
       (debug_get_hardfork_property_object)
     )

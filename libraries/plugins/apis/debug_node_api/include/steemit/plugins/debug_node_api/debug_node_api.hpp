#pragma once
#include <steemit/plugins/json_rpc/utility.hpp>
#include <steemit/plugins/database_api/database_api_objects.hpp>

#include <steemit/protocol/types.hpp>

#include <fc/optional.hpp>
#include <fc/variant.hpp>
#include <fc/vector.hpp>

namespace steemit { namespace plugins { namespace debug_node {

using json_rpc::void_type;

namespace detail { class debug_node_api_impl; }

struct debug_push_blocks_args
{
   std::string                               src_filename;
   uint32_t                                  count;
   bool                                      skip_validate_invariants = false;
};

struct debug_push_blocks_return
{
   uint32_t                                  blocks;
};

struct debug_generate_blocks_args
{
   std::string                               debug_key;
   uint32_t                                  count;
};

typedef debug_push_blocks_return debug_generate_blocks_return;

struct debug_generate_blocks_until_args
{
   std::string                               debug_key;
   fc::time_point_sec                        head_block_time;
   bool                                      generate_sparsely = true;
};

typedef debug_push_blocks_return debug_generate_blocks_until_return;

typedef void_type debug_pop_block_args;

struct debug_pop_block_return
{
   fc::optional< protocol::signed_block > block;
};

typedef void_type debug_get_witness_schedule_args;
typedef database_api::api_witness_schedule_object debug_get_witness_schedule_return;

typedef void_type debug_get_hardfork_property_object_args;
typedef database_api::api_hardfork_property_object debug_get_hardfork_property_object_return;

struct debug_set_dev_key_prefix_args
{
   std::string prefix;
};

typedef void_type debug_set_dev_key_prefix_return;

struct debug_get_dev_key_args
{
   std::string                               name;
};

struct debug_get_dev_key_return
{
   std::string                               private_key;
   chain::public_key_type                    public_key;
};

struct debug_mine_args
{
   std::string                               worker_account;
   fc::optional< chain::chain_properties >   props;
};

typedef void_type debug_mine_return;

struct debug_set_hardfork_args
{
   uint32_t hardfork_id;
};

typedef void_type debug_set_hardfork_return;

typedef debug_set_hardfork_args debug_has_hardfork_args;

struct debug_has_hardfork_return
{
   bool has_hardfork;
};

typedef void_type debug_get_json_schema_args;

struct debug_get_json_schema_return
{
   std::string schema;
};


class debug_node_api
{
   public:
      debug_node_api();

      /**
       * Push blocks from existing database.
       */
      DECLARE_API( debug_push_blocks )

      /**
       * Generate blocks locally.
       */
      DECLARE_API( debug_generate_blocks )

      /*
       * Generate blocks locally until a specified head block time. Can generate them sparsely.
       */
      DECLARE_API( debug_generate_blocks_until )

      /*
       * Pop a block from the blockchain, returning it
       */
      DECLARE_API( debug_pop_block )

      /*
       * Push an already constructed block onto the blockchain. For use with pop_block to traverse state block by block.
       */
      // not implemented
      //void debug_push_block( steemit::chain::signed_block& block );

      DECLARE_API( debug_get_witness_schedule )

      DECLARE_API( debug_get_hardfork_property_object )

      /**
       * Directly manipulate database objects (will undo and re-apply last block with new changes post-applied).
       */
      //void debug_update_object( fc::variant_object update );

      //fc::variant_object debug_get_edits();

      //void debug_set_edits( fc::variant_object edits );

      /**
       * Set developer key prefix. This prefix only applies to the current API session.
       * (Thus, this method is only useful to websocket-based API clients.)
       * Prefix will be used for debug_get_dev_key() and debug_mine_account().
       */
      DECLARE_API( debug_set_dev_key_prefix )

      /**
       * Get developer key. Use debug_set_key_prefix() to set a prefix if desired.
       */
      DECLARE_API( debug_get_dev_key )

      /**
       * Synchronous mining, does not return until work is found.
       */
      DECLARE_API( debug_mine )

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

      //void debug_stream_json_objects( std::string filename );

      /**
       * Flush streaming file.
       */
      //void debug_stream_json_objects_flush();

      DECLARE_API( debug_set_hardfork )

      DECLARE_API( debug_has_hardfork )

      DECLARE_API( debug_get_json_schema )

   private:
      std::shared_ptr< detail::debug_node_api_impl > my;
};

} } } // steemit::plugins::debug_node

FC_REFLECT( steemit::plugins::debug_node::debug_push_blocks_args,
            (src_filename)(count)(skip_validate_invariants) )

FC_REFLECT( steemit::plugins::debug_node::debug_push_blocks_return,
            (blocks) )

FC_REFLECT( steemit::plugins::debug_node::debug_generate_blocks_args,
            (debug_key)(count) )

FC_REFLECT( steemit::plugins::debug_node::debug_generate_blocks_until_args,
            (debug_key)(head_block_time)(generate_sparsely) )

FC_REFLECT( steemit::plugins::debug_node::debug_pop_block_return,
            (block) )

FC_REFLECT( steemit::plugins::debug_node::debug_set_dev_key_prefix_args,
            (prefix) )

FC_REFLECT( steemit::plugins::debug_node::debug_get_dev_key_args,
            (name) )

FC_REFLECT( steemit::plugins::debug_node::debug_get_dev_key_return,
            (private_key)(public_key) )

FC_REFLECT( steemit::plugins::debug_node::debug_mine_args,
            (worker_account)(props) )

FC_REFLECT( steemit::plugins::debug_node::debug_set_hardfork_args,
            (hardfork_id) )

FC_REFLECT( steemit::plugins::debug_node::debug_has_hardfork_return,
            (has_hardfork) )

FC_REFLECT( steemit::plugins::debug_node::debug_get_json_schema_return,
            (schema) )

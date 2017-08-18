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

      DECLARE_API(
         /**
         * Push blocks from existing database.
         */
         (debug_push_blocks)

         /**
         * Generate blocks locally.
         */
         (debug_generate_blocks)

         /*
         * Generate blocks locally until a specified head block time. Can generate them sparsely.
         */
         (debug_generate_blocks_until)

         /*
         * Pop a block from the blockchain, returning it
         */
         (debug_pop_block)
         (debug_get_witness_schedule)
         (debug_get_hardfork_property_object)

         /**
         * Set developer key prefix. This prefix only applies to the current API session.
         * (Thus, this method is only useful to websocket-based API clients.)
         * Prefix will be used for debug_get_dev_key() and debug_mine_account().
         */
         (debug_set_dev_key_prefix)

         /**
         * Get developer key. Use debug_set_key_prefix() to set a prefix if desired.
         */
         (debug_get_dev_key)

         (debug_set_hardfork)
         (debug_has_hardfork)
         (debug_get_json_schema)
      )

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

FC_REFLECT( steemit::plugins::debug_node::debug_set_hardfork_args,
            (hardfork_id) )

FC_REFLECT( steemit::plugins::debug_node::debug_has_hardfork_return,
            (has_hardfork) )

FC_REFLECT( steemit::plugins::debug_node::debug_get_json_schema_return,
            (schema) )

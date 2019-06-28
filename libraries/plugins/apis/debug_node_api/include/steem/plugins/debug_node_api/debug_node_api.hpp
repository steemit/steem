#pragma once
#include <steem/plugins/json_rpc/utility.hpp>
#include <steem/plugins/database_api/database_api_objects.hpp>
#include <steem/plugins/debug_node/debug_node_plugin.hpp>

#include <steem/protocol/types.hpp>

#include <fc/optional.hpp>
#include <fc/variant.hpp>
#include <fc/vector.hpp>

namespace steem { namespace plugins { namespace debug_node {

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
      ~debug_node_api();

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

         (debug_set_hardfork)
         (debug_has_hardfork)
         (debug_get_json_schema)
      )

   private:
      std::unique_ptr< detail::debug_node_api_impl > my;
};

} } } // steem::plugins::debug_node

FC_REFLECT( steem::plugins::debug_node::debug_push_blocks_args,
            (src_filename)(count)(skip_validate_invariants) )

FC_REFLECT( steem::plugins::debug_node::debug_push_blocks_return,
            (blocks) )

FC_REFLECT( steem::plugins::debug_node::debug_generate_blocks_until_args,
            (debug_key)(head_block_time)(generate_sparsely) )

FC_REFLECT( steem::plugins::debug_node::debug_pop_block_return,
            (block) )

FC_REFLECT( steem::plugins::debug_node::debug_set_hardfork_args,
            (hardfork_id) )

FC_REFLECT( steem::plugins::debug_node::debug_has_hardfork_return,
            (has_hardfork) )

FC_REFLECT( steem::plugins::debug_node::debug_get_json_schema_return,
            (schema) )

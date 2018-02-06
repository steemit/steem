#pragma once
#include <golos/plugins/chain/plugin.hpp>
#include <appbase/application.hpp>

#include <fc/variant_object.hpp>

#include <map>
// #include <fstream>

#include <boost/program_options.hpp>
#include <golos/plugins/chain/plugin.hpp>
#include <golos/plugins/json_rpc/utility.hpp>
#include <golos/plugins/json_rpc/plugin.hpp>

namespace golos { namespace protocol {
    struct pow2;
    struct signed_block;
} }

namespace golos { namespace plugins { namespace debug_node {

inline void disconnect_signal( boost::signals2::connection& signal )
{
   if( signal.connected() ) {
      signal.disconnect();
   }
   FC_ASSERT( !signal.connected() );
}

using golos::plugins::json_rpc::void_type;
using golos::plugins::json_rpc::msg_pack;


// debug_generate_blocks
struct debug_generate_blocks_a {
    std::string                               debug_key;
    uint32_t                                  count = 0;
    uint32_t                                  skip = golos::chain::database::skip_nothing;
    uint32_t                                  miss_blocks = 0;
    bool                                      edit_if_needed = true;
};

struct debug_generate_blocks_r {
   uint32_t                                  blocks = 0;
};


// debug_push_blocks
struct debug_push_blocks_a
{
    std::string                               src_filename;
    uint32_t                                  count;
    bool                                      skip_validate_invariants = false;
};

struct debug_push_blocks_r
{
    uint32_t                                  blocks;
};


// debug_generate_blocks_until
struct debug_generate_blocks_until_a
{
    std::string                               debug_key;
    fc::time_point_sec                        head_block_time;
    bool                                      generate_sparsely = true;
};

typedef debug_push_blocks_r debug_generate_blocks_until_r;


// debug_pop_block
typedef void_type debug_pop_block_a;
// struct debug_pop_block_a {

// };

struct debug_pop_block_r {
    fc::optional< protocol::signed_block > block;
};

using golos::chain::witness_schedule_object;

// debug_get_witness_schedule
typedef void_type debug_get_witness_schedule_a;
typedef witness_schedule_object debug_get_witness_schedule_r;


// debug_get_hardfork_property_object

// typedef void_type debug_get_hardfork_property_object_a;
// typedef hardfork_property_api_object debug_get_hardfork_property_object_r;

// debug_set_hardfork
struct debug_set_hardfork_a {
   uint32_t hardfork_id;
};

typedef void_type debug_set_hardfork_r;


// debug_set_hardfork
typedef debug_set_hardfork_a debug_has_hardfork_a;

struct debug_has_hardfork_r {
   bool has_hardfork;
};


// DEFINE API ARGS
DEFINE_API_ARGS ( debug_generate_blocks,              msg_pack,   debug_generate_blocks_r               )
DEFINE_API_ARGS ( debug_generate_blocks_until,        msg_pack,   debug_generate_blocks_until_r         )
DEFINE_API_ARGS ( debug_push_blocks,                  msg_pack,   debug_push_blocks_r                   )
DEFINE_API_ARGS ( debug_pop_block,                    msg_pack,   debug_pop_block_r                     )
DEFINE_API_ARGS ( debug_get_witness_schedule,         msg_pack,   debug_get_witness_schedule_r          )
// DEFINE_API_ARGS ( debug_get_hardfork_property_object, msg_pack,   debug_get_hardfork_property_object_r  )
DEFINE_API_ARGS ( debug_set_hardfork,                 msg_pack,   debug_set_hardfork_r                  )
DEFINE_API_ARGS ( debug_has_hardfork,                 msg_pack,   debug_has_hardfork_r                  );
// 


class plugin final : public appbase::plugin<plugin> {
public:
    APPBASE_PLUGIN_REQUIRES( (golos::plugins::chain::plugin) )

    constexpr const static char *plugin_name = "debug_node";

    static const std::string &name() {
        static std::string name = plugin_name;
        return name;
    }

    plugin();

    ~plugin();

    void set_program_options (
        boost::program_options::options_description &cli,
        boost::program_options::options_description &cfg
    ) override ;

    virtual void plugin_initialize( const boost::program_options::variables_map& options ) override;
    virtual void plugin_startup() override;
    virtual void plugin_shutdown() override;

    DECLARE_API(
        /**
        * Push blocks from existing database.
        */
        (debug_push_blocks)

        /**
        * Generate blocks locally.
        */
        (debug_generate_blocks)

        /**
        * Generate blocks locally until a specified head block time. Can generate them sparsely.
        */
        (debug_generate_blocks_until)

        /**
        * Pop a block from the blockchain, returning it
        */
        (debug_pop_block)
        (debug_get_witness_schedule)
        // (debug_get_hardfork_property_object)

        (debug_set_hardfork)
        (debug_has_hardfork)
    )

    // golos::chain::database& database();

    void save_debug_updates( fc::mutable_variant_object& target );
    void load_debug_updates( const fc::variant_object& target );


private:
    void on_applied_block( const protocol::signed_block& b );

    void apply_debug_updates();

    std::map<protocol::public_key_type, fc::ecc::private_key> _private_keys;


    // boost::signals2::scoped_connection _applied_block_conn;
    // boost::signals2::scoped_connection _changed_objects_conn;
    // boost::signals2::scoped_connection _removed_objects_conn;

    std::vector< std::string > _edit_scripts;
    //std::map< protocol::block_id_type, std::vector< fc::variant_object > > _debug_updates;

    struct plugin_impl;

    std::unique_ptr<plugin_impl> my;
};

} } } // golos::plugins::debug_node

FC_REFLECT( (golos::plugins::debug_node::debug_generate_blocks_a),
            (debug_key)
            (count)
            (skip)
            (miss_blocks)
            (edit_if_needed)
          )
FC_REFLECT( (golos::plugins::debug_node::debug_generate_blocks_r),
            (blocks)
          )

FC_REFLECT( (golos::plugins::debug_node::debug_push_blocks_a),
            (src_filename)(count)(skip_validate_invariants)
          )

FC_REFLECT( (golos::plugins::debug_node::debug_push_blocks_r),
            (blocks)
          )

FC_REFLECT( (golos::plugins::debug_node::debug_generate_blocks_until_a),
            (debug_key)(head_block_time)(generate_sparsely)
          )

// FC_REFLECT( (golos::plugins::debug_node::debug_pop_block_a),
            // ()
          // )

FC_REFLECT( (golos::plugins::debug_node::debug_pop_block_r),
            (block)
          )

FC_REFLECT( (golos::plugins::debug_node::debug_set_hardfork_a),
            (hardfork_id)
          )

FC_REFLECT( (golos::plugins::debug_node::debug_has_hardfork_r),
            (has_hardfork)
          )

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

using golos::plugins::json_rpc::void_type;
using golos::plugins::json_rpc::msg_pack;
using golos::chain::witness_schedule_object;

// DEFINE API ARGS
DEFINE_API_ARGS ( debug_generate_blocks,              msg_pack,   uint32_t                                      )
DEFINE_API_ARGS ( debug_generate_blocks_until,        msg_pack,   uint32_t                                      )
DEFINE_API_ARGS ( debug_push_blocks,                  msg_pack,   uint32_t                                      )
DEFINE_API_ARGS ( debug_push_json_blocks,             msg_pack,   uint32_t                                      )
DEFINE_API_ARGS ( debug_pop_block,                    msg_pack,   fc::optional< protocol::signed_block >        )
DEFINE_API_ARGS ( debug_get_witness_schedule,         msg_pack,   witness_schedule_object                       )
// DEFINE_API_ARGS ( debug_get_hardfork_property_object, msg_pack,   debug_get_hardfork_property_object_r  )
DEFINE_API_ARGS ( debug_set_hardfork,                 msg_pack,   void_type                                     )
DEFINE_API_ARGS ( debug_has_hardfork,                 msg_pack,   bool                                          );
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

        /* Push blocks from json file (array of signed blocks)*/
        (debug_push_json_blocks)

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
    void debug_update( 
        std::function< void( golos::chain::database& ) > callback,
        uint32_t skip = golos::chain::database::skip_nothing
    );

    void set_logging(const bool islogging);


private:
    struct plugin_impl;

    std::unique_ptr<plugin_impl> my;
};

} } } // golos::plugins::debug_node

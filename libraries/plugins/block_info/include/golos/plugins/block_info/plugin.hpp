#pragma once

#include <appbase/application.hpp>
#include <golos/plugins/block_info/block_info.hpp>

#include <boost/program_options.hpp>
#include <string>
#include <vector>
#include <golos/plugins/chain/plugin.hpp>
#include <golos/plugins/json_rpc/utility.hpp>
#include <golos/plugins/json_rpc/plugin.hpp>

namespace golos {
namespace protocol {

struct signed_block;

} }


namespace golos {
namespace plugins {
namespace block_info {

using golos::plugins::json_rpc::msg_pack;

struct get_block_info_a {
    uint32_t start_block_num = 0;
    uint32_t count = 1000;
};

struct get_blocks_with_info_a {
    uint32_t start_block_num = 0;
    uint32_t count = 1000;
};    

struct get_block_info_r {
    std::vector<block_info> block_info_vec;
};

struct get_blocks_with_info_r {
    std::vector<block_with_info> block_with_info_vec;
};

DEFINE_API_ARGS ( get_block_info,           msg_pack,       get_block_info_r       )
DEFINE_API_ARGS ( get_blocks_with_info,     msg_pack,       get_blocks_with_info_r )


using boost::program_options::options_description;

class plugin final : public appbase::plugin<plugin> {
public:
    APPBASE_PLUGIN_REQUIRES((golos::plugins::chain::plugin))

    constexpr const static char *plugin_name = "block_info";

    static const std::string &name() {
        static std::string name = plugin_name;
        return name;
    }

    plugin();

    ~plugin();

    void set_program_options(boost::program_options::options_description &cli, boost::program_options::options_description &cfg) override {
    }

    void plugin_initialize(const boost::program_options::variables_map &options) override;

    void plugin_startup() override;

    void plugin_shutdown() override;

    DECLARE_API(
            (get_block_info)
            (get_blocks_with_info)
    )
    void on_applied_block(const protocol::signed_block &b);

private:
    struct plugin_impl;

    std::unique_ptr<plugin_impl> my;
};

} } }

FC_REFLECT((golos::plugins::block_info::get_block_info_a),
        (start_block_num)(count)
)

FC_REFLECT((golos::plugins::block_info::get_blocks_with_info_a),
        (start_block_num)(count)
)

FC_REFLECT((golos::plugins::block_info::get_block_info_r),
        (block_info_vec)
)

FC_REFLECT((golos::plugins::block_info::get_blocks_with_info_r),
        (block_with_info_vec)
)

#pragma once

#include <string>
#include <vector>
#include <boost/program_options.hpp>
#include <appbase/application.hpp>
#include <golos/plugins/chain/plugin.hpp>
#include <golos/plugins/json_rpc/utility.hpp>
#include <golos/plugins/json_rpc/plugin.hpp>

namespace golos     {
namespace plugins   {
namespace raw_block {

using golos::plugins::json_rpc::msg_pack;

struct get_raw_block_a {
    uint32_t block_num = 0;
};

struct get_raw_block_r {
    golos::chain::block_id_type block_id;
    golos::chain::block_id_type previous;
    fc::time_point_sec timestamp;
    std::string raw_block;
};

DEFINE_API_ARGS ( get_raw_block, msg_pack, get_raw_block_r )

using boost::program_options::options_description;

class plugin final : public appbase::plugin<plugin> {
public:
    APPBASE_PLUGIN_REQUIRES(
        (chain::plugin)
        (json_rpc::plugin)
    )

    constexpr const static char *plugin_name = "raw_block";

    static const std::string &name() {
        static std::string name = plugin_name;
        return name;
    }

    plugin();

    ~plugin();

    void set_program_options(
        boost::program_options::options_description &cli,
        boost::program_options::options_description &cfg) override {
    }

    void plugin_initialize(const boost::program_options::variables_map &options) override;

    void plugin_startup() override;

    void plugin_shutdown() override;

    DECLARE_API (
        (get_raw_block)
    )

private:
    struct plugin_impl;

    std::unique_ptr<plugin_impl> my;
};

} } } // golos::plugins::raw_block

FC_REFLECT((golos::plugins::raw_block::get_raw_block_a),
    (block_num)
)

FC_REFLECT((golos::plugins::raw_block::get_raw_block_r),
    (block_id)(previous)(timestamp)(raw_block)
)
    
#include <golos/chain/database.hpp>

#include <golos/plugins/block_info/plugin.hpp>

#include <golos/protocol/types.hpp>
#include <golos/plugins/json_rpc/utility.hpp>
#include <golos/plugins/json_rpc/plugin.hpp>

namespace golos {
namespace plugins {
namespace block_info {

using namespace golos::chain;

struct plugin::plugin_impl  {
public:
    plugin_impl() : db_(appbase::app().get_plugin<plugins::chain::plugin>().db()) {
    }

    // API
    get_block_info_r get_block_info(const get_block_info_a & args);
    get_blocks_with_info_r get_blocks_with_info(const get_blocks_with_info_a & args);

    // PLUGIN_METHODS
    void on_applied_block(const protocol::signed_block &b);

    // HELPING METHODS
    golos::chain::database &database() {
        return db_;
    }
// protected:
    boost::signals2::scoped_connection applied_block_conn_;
private:
    std::vector<block_info> block_info_;

    golos::chain::database & db_;
};

get_block_info_r plugin::plugin_impl::get_block_info(const get_block_info_a & args) {
    get_block_info_r result;

    FC_ASSERT(args.start_block_num > 0);
    FC_ASSERT(args.count <= 10000);
    uint32_t n = std::min(uint32_t(block_info_.size()),
    args.start_block_num + args.count);

    for (uint32_t block_num = args.start_block_num;
        block_num < n; block_num++) {
        result.block_info_vec.emplace_back(block_info_[block_num]);
    }

    return result;
}

get_blocks_with_info_r plugin::plugin_impl::get_blocks_with_info(const get_blocks_with_info_a & args) {    
    get_blocks_with_info_r result;
    const auto & db = database();

    FC_ASSERT(args.start_block_num > 0);
    FC_ASSERT(args.count <= 10000);
    uint32_t n = std::min( uint32_t( block_info_.size() ), args.start_block_num + args.count );

    uint64_t total_size = 0;
    for (uint32_t block_num = args.start_block_num;
         block_num < n; block_num++) {
        uint64_t new_size =
                total_size + block_info_[block_num].block_size;
        if ((new_size > 8 * 1024 * 1024) &&
            (block_num != args.start_block_num)) {
                break;
        }
        total_size = new_size;
        result.block_with_info_vec.emplace_back();
        result.block_with_info_vec.back().block = *db.fetch_block_by_number(block_num);
        result.block_with_info_vec.back().info = block_info_[block_num];
    }

    return result;
}

void plugin::plugin_impl::on_applied_block(const protocol::signed_block &b) {
    uint32_t block_num = b.block_num();
    const auto &db = appbase::app().get_plugin<chain::plugin>().db();

    while (block_num >= block_info_.size()) {
        block_info_.emplace_back();
    }

    block_info &info = block_info_[block_num];
    const dynamic_global_property_object &dgpo = db.get_dynamic_global_properties();

    info.block_id = b.id();
    info.block_size = fc::raw::pack_size(b);
    info.average_block_size = dgpo.average_block_size;
    info.aslot = dgpo.current_aslot;
    info.last_irreversible_block_num = dgpo.last_irreversible_block_num;
    info.num_pow_witnesses = dgpo.num_pow_witnesses;
    return;
}


DEFINE_API ( plugin, get_block_info ) {
    auto tmp = args.args->at(0).as<get_block_info_a>();
    auto &db = my->database();
    return db.with_read_lock([&]() {
        return  my->get_block_info(tmp);;
    });
}

DEFINE_API ( plugin, get_blocks_with_info ) {
    auto tmp = args.args->at(0).as<get_blocks_with_info_a>();
    auto &db = my->database();
    return db.with_read_lock([&]() {
        return my->get_blocks_with_info(tmp);
    });
}

void plugin::on_applied_block(const protocol::signed_block &b) {
    return my->on_applied_block(b);
}


plugin::plugin() {
}

plugin::~plugin() {
}

void plugin::plugin_initialize(const boost::program_options::variables_map &options) {

    auto &db = appbase::app().get_plugin<chain::plugin>().db();

    my.reset(new plugin_impl);

    my->applied_block_conn_ = db.applied_block.connect([this](const protocol::signed_block &b) {
        on_applied_block(b);
    });

    JSON_RPC_REGISTER_API ( name() ) ;
}

void plugin::plugin_startup() {
}

void plugin::plugin_shutdown() {
}

} } } // golos::plugin::block_info

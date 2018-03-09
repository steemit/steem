#include <golos/plugins/raw_block/plugin.hpp>
#include <golos/chain/database.hpp>
#include <golos/protocol/types.hpp>
#include <golos/plugins/json_rpc/utility.hpp>
#include <golos/plugins/json_rpc/plugin.hpp>

namespace golos {
namespace plugins {
namespace raw_block {

struct plugin::plugin_impl {
public:
    plugin_impl() : db_(appbase::app().get_plugin<plugins::chain::plugin>().db()) {
    }
     // API
    get_raw_block_r get_raw_block(uint32_t block_num = 0);

    // HELPING METHODS
    golos::chain::database &database() {
        return db_;
    }
private:
    golos::chain::database & db_;
};

get_raw_block_r plugin::plugin_impl::get_raw_block(uint32_t block_num) {
    get_raw_block_r result;
    const auto &db = database();

    auto block = db.fetch_block_by_number(block_num);
    if (!block.valid()) {
        return result;
    }
    std::vector<char> serialized_block = fc::raw::pack(*block);
    result.raw_block = fc::base64_encode(
        std::string(
            &serialized_block[0],
            &serialized_block[0] + serialized_block.size()
        )
    );
    result.block_id = block->id();
    result.previous = block->previous;
    result.timestamp = block->timestamp;
    return result;
}

DEFINE_API ( plugin, get_raw_block ) {
    auto tmp = args.args->at(0).as<uint32_t>();
    auto &db = my->database();
    return db.with_read_lock([&]() {
        return my->get_raw_block(tmp);
    });
}

plugin::plugin() {
}

plugin::~plugin() {
}

void plugin::plugin_initialize(const boost::program_options::variables_map &options) {
    my.reset(new plugin_impl);
    JSON_RPC_REGISTER_API ( name() ) ;
}

void plugin::plugin_startup() {
}

void plugin::plugin_shutdown() {
}

} } } // golos::plugin::raw_block

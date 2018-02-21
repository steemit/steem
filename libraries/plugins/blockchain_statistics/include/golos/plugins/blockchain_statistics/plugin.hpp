#pragma once

#include <appbase/application.hpp>
#include <golos/plugins/chain/plugin.hpp>

#include <golos/chain/steem_object_types.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/program_options.hpp>

#include <golos/plugins/blockchain_statistics/bucket_object.hpp>

//
// Plugins should #define their SPACE_ID's so plugins with
// conflicting SPACE_ID assignments can be compiled into the
// same binary (by simply re-assigning some of the conflicting #defined
// SPACE_ID's in a build script).
//
// Assignment of SPACE_ID's cannot be done at run-time because
// various template automagic depends on them being known at compile
// time.
//
#ifndef BLOCKCHAIN_STATISTICS_SPACE_ID
#define BLOCKCHAIN_STATISTICS_SPACE_ID 9
#endif

#ifndef BLOCKCHAIN_STATISTICS_PLUGIN_NAME
#define BLOCKCHAIN_STATISTICS_PLUGIN_NAME "chain_stats"
#endif

namespace golos {
namespace plugins {
namespace blockchain_statistics {

using namespace golos::chain;
using boost::program_options::options_description;
using boost::program_options::variables_map;


class plugin final : public appbase::plugin<plugin> {
public:
    static const std::string &name() {
        static std::string name = BLOCKCHAIN_STATISTICS_PLUGIN_NAME;
        return name;
    }

    APPBASE_PLUGIN_REQUIRES((chain::plugin))

    plugin();

    ~plugin();

    void set_program_options( options_description& cli, options_description& cfg ) override ;

    void plugin_initialize(const boost::program_options::variables_map &options) override;

    void plugin_startup() override;

    const flat_set<uint32_t> &get_tracked_buckets() const;

    uint32_t get_max_history_per_bucket() const;

    void plugin_shutdown() override;

private:
    struct plugin_impl;

    std::unique_ptr<plugin_impl> _my;
};

} } } // golos::plugins::blockchain_statistics

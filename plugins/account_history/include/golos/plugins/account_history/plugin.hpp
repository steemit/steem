/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once

#include <appbase/application.hpp>
#include <golos/plugins/chain/plugin.hpp>

#include <golos/chain/database.hpp>
#include <boost/program_options.hpp>

#include <fc/thread/future.hpp>

#include <golos/plugins/json_rpc/utility.hpp>
#include <golos/plugins/json_rpc/plugin.hpp>
#include <golos/chain/applied_operation.hpp>
#include <golos/plugins/account_history/history_object.hpp>


namespace golos {
namespace plugins {
namespace account_history {
using namespace chain;
using golos::chain::applied_operation;
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

#ifndef ACCOUNT_HISTORY_SPACE_ID
#define ACCOUNT_HISTORY_SPACE_ID 5
#endif

enum account_history_object_type {
    key_account_object_type = 0,
    bucket_object_type = 1 
};

using get_account_history_return_type = std::map<uint32_t, applied_operation>;


using plugins::json_rpc::void_type;
using plugins::json_rpc::msg_pack;
using plugins::json_rpc::msg_pack_transfer;

DEFINE_API_ARGS(get_account_history, msg_pack, get_account_history_return_type);

/**
*  This plugin is designed to track a range of operations by account so that one node
*  doesn't need to hold the full operation history in memory.
*/
class plugin : public appbase::plugin<plugin> {
public:
    constexpr static const char *plugin_name = "account_history";

    APPBASE_PLUGIN_REQUIRES(
        (json_rpc::plugin)
        (chain::plugin)
    )

    static const std::string &name() {
        static std::string name = plugin_name;
        return name;
    }


    plugin( );
    ~plugin( );

    void set_program_options(boost::program_options::options_description &cli,
                             boost::program_options::options_description &cfg) override;

    void plugin_initialize(const boost::program_options::variables_map &options) override;

    void plugin_startup() override;

    void plugin_shutdown() override {
    }

    flat_map<string, string> tracked_accounts() const; /// map start_range to end_range
    /**
     *  Account operations have sequence numbers from 0 to N where N is the most recent operation. This method
     *  returns operations in the range [from-limit, from]
     *
     *  @param from - the absolute sequence number, -1 means most recent, limit is the number of operations before from.
     *  @param limit - the maximum number of items that can be queried (0 to 1000], must be less than from
     */
    DECLARE_API(
        (get_account_history)
    )
private:
    struct plugin_impl;

    std::unique_ptr<plugin_impl> my;
};

}
}
} // golos::plugins::account_history

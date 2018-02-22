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

namespace golos {
namespace plugins {
namespace account_history {
using namespace chain;
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
    bucket_object_type = 1 ///< used in market_history_plugin
};

/**
*  This plugin is designed to track a range of operations by account so that one node
*  doesn't need to hold the full operation history in memory.
*/
class plugin : public appbase::plugin<plugin> {
public:
    constexpr static const char *plugin_name = "account_history";

    APPBASE_PLUGIN_REQUIRES((chain::plugin))

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

private:
    struct plugin_impl;

    std::unique_ptr<plugin_impl> my;
};

}
}
} // golos::plugins::account_history

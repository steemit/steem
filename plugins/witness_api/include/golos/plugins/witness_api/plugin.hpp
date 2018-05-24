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
#include <golos/plugins/witness_api/api_objects/feed_history_api_object.hpp>
#include <golos/api/witness_api_object.hpp>

#include <golos/chain/database.hpp>

#include <golos/plugins/json_rpc/utility.hpp>
#include <golos/plugins/json_rpc/plugin.hpp>
#include <boost/optional.hpp>


namespace golos { namespace plugins { namespace witness_api {

using namespace chain;
using namespace golos::protocol;
using namespace golos::api;
using plugins::json_rpc::msg_pack;


DEFINE_API_ARGS(get_current_median_history_price, msg_pack, price)
DEFINE_API_ARGS(get_feed_history,                 msg_pack, feed_history_api_object)
DEFINE_API_ARGS(get_miner_queue,                  msg_pack, std::vector<account_name_type>)
DEFINE_API_ARGS(get_active_witnesses,             msg_pack, std::vector<account_name_type>)
DEFINE_API_ARGS(get_witness_schedule,             msg_pack, golos::chain::witness_schedule_object)
DEFINE_API_ARGS(get_witnesses,                    msg_pack, std::vector<optional<witness_api_object> >)
DEFINE_API_ARGS(get_witness_by_account,           msg_pack, optional<witness_api_object>)
DEFINE_API_ARGS(get_witnesses_by_vote,            msg_pack, std::vector<witness_api_object>)
DEFINE_API_ARGS(get_witness_count,                msg_pack, uint64_t)
DEFINE_API_ARGS(lookup_witness_accounts,          msg_pack, std::set<account_name_type>)


class plugin : public appbase::plugin<plugin> {
public:
    constexpr static const char *plugin_name = "witness_api";

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

    DECLARE_API(
        (get_current_median_history_price)
        (get_feed_history)
        (get_miner_queue)
        (get_active_witnesses)
        (get_witness_schedule)
        (get_witnesses)
        (get_witness_by_account)
        (get_witnesses_by_vote)
        (get_witness_count)
        (lookup_witness_accounts)
    )
private:
    struct witness_plugin_impl;

    std::unique_ptr<witness_plugin_impl> my;
};

} } } // golos::plugins::witness_api

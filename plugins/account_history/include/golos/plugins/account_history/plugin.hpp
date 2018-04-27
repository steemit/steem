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
#include <golos/plugins/account_history/applied_operation.hpp>
#include <golos/plugins/account_history/history_object.hpp>


namespace golos {
namespace plugins {
namespace account_history {
using namespace chain;

using get_account_history_return_type = std::map<uint32_t, applied_operation>;


using plugins::json_rpc::void_type;
using plugins::json_rpc::msg_pack;
using plugins::json_rpc::msg_pack_transfer;

DEFINE_API_ARGS(get_account_history,              msg_pack, get_account_history_return_type)
DEFINE_API_ARGS(get_ops_in_block,                 msg_pack, std::vector<applied_operation>)
DEFINE_API_ARGS(get_transaction,                  msg_pack, annotated_signed_transaction)

///account_history_api
struct operation_api_object {
    operation_api_object() {
    }

    operation_api_object(const operation_object &op_obj) : trx_id(op_obj.trx_id),
            block(op_obj.block), trx_in_block(op_obj.trx_in_block), virtual_op(op_obj.virtual_op),
            timestamp(op_obj.timestamp) {
        op = fc::raw::unpack<golos::protocol::operation>(op_obj.serialized_op);
    }

    golos::protocol::transaction_id_type trx_id;
    uint32_t block = 0;
    uint32_t trx_in_block = 0;
    uint16_t op_in_trx = 0;
    uint64_t virtual_op = 0;
    fc::time_point_sec timestamp;
    golos::protocol::operation op;
};

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
    DECLARE_API(
        /**
         *  Account operations have sequence numbers from 0 to N where N is the most recent operation. This method
         *  returns operations in the range [from-limit, from]
         *
         *  @param from - the absolute sequence number, -1 means most recent, limit is the number of operations before from.
         *  @param limit - the maximum number of items that can be queried (0 to 1000], must be less than from
         */
        (get_account_history)

        /**
         *  @brief Get sequence of operations included/generated within a particular block
         *  @param block_num Height of the block whose generated virtual operations should be returned
         *  @param only_virtual Whether to only include virtual operations in returned results (default: true)
         *  @return sequence of operations included/generated within the block
         */
        (get_ops_in_block)
        
        (get_transaction)

    )
private:
    struct plugin_impl;

    std::unique_ptr<plugin_impl> my;
};

}
}
} // golos::plugins::account_history


// FC_REFLECT((golos::plugins::account_history::operation_api_object),
//            (trx_id)(block)(trx_in_block)(op_in_trx)(virtual_op)(timestamp)(op)
//            )
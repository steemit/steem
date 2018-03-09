#pragma once

#include <golos/plugins/market_history/market_history_objects.hpp>

#include <appbase/plugin.hpp>

#include <golos/chain/steem_object_types.hpp>

#include <boost/multi_index/composite_key.hpp>
#include <appbase/application.hpp>

#include <golos/plugins/json_rpc/utility.hpp>
#include <golos/plugins/json_rpc/plugin.hpp>
#include <golos/protocol/asset.hpp>
#include <golos/protocol/steem_virtual_operations.hpp>
#include <golos/chain/operation_notification.hpp>
#include <golos/chain/steem_objects.hpp>
#include <golos/protocol/types.hpp>
#include <golos/plugins/chain/plugin.hpp>
#include <chainbase/chainbase.hpp>


namespace golos {
    namespace plugins {
        namespace market_history {

            using namespace chain;
            using appbase::application;
            using namespace chainbase;
            using namespace golos::protocol;

            DEFINE_API_ARGS(get_ticker,                 json_rpc::msg_pack, market_ticker)
            DEFINE_API_ARGS(get_volume,                 json_rpc::msg_pack, market_volume)
            DEFINE_API_ARGS(get_order_book,             json_rpc::msg_pack, order_book)
            DEFINE_API_ARGS(get_trade_history,          json_rpc::msg_pack, vector<market_trade>)
            DEFINE_API_ARGS(get_recent_trades,          json_rpc::msg_pack, vector<market_trade>)
            DEFINE_API_ARGS(get_market_history,         json_rpc::msg_pack, vector<bucket_object>)
            DEFINE_API_ARGS(get_market_history_buckets, json_rpc::msg_pack, flat_set<uint32_t>)
            DEFINE_API_ARGS(get_open_orders,            json_rpc::msg_pack, std::vector<limit_order>)

            class market_history_plugin : public appbase::plugin<market_history_plugin> {
            public:

                APPBASE_PLUGIN_REQUIRES((json_rpc::plugin))

                market_history_plugin();

                virtual ~market_history_plugin();

                void set_program_options(
                        boost::program_options::options_description &cli,
                        boost::program_options::options_description &cfg) override;

                void plugin_initialize(const boost::program_options::variables_map &options) override;

                void plugin_startup() override;

                void plugin_shutdown() override;

                flat_set<uint32_t> get_tracked_buckets() const;

                uint32_t get_max_history_per_bucket() const;

                DECLARE_API((get_ticker)
                                (get_volume)
                                (get_order_book)
                                (get_trade_history)
                                (get_recent_trades)
                                (get_market_history)
                                (get_market_history_buckets)
                                (get_open_orders))

                constexpr const static char *plugin_name = "market_history";

                static const std::string &name() {
                    static std::string name = plugin_name;
                    return name;
                }

            private:
                class market_history_plugin_impl;

                std::unique_ptr<market_history_plugin_impl> _my;
            };
        }
    }
} // golos::plugins::market_history



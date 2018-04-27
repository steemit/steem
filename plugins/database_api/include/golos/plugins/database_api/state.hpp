#pragma once

#include <golos/chain/global_property_object.hpp>
#include <golos/chain/account_object.hpp>
#include <golos/chain/steem_objects.hpp>
#include <golos/chain/steem_object_types.hpp>
#include <golos/plugins/database_api/api_objects/account_api_object.hpp>
#include <golos/plugins/database_api/api_objects/witness_api_object.hpp>
#include "forward.hpp"

namespace golos {
    namespace plugins {
        namespace database_api {
            using std::string;
            using std::vector;

            struct extended_limit_order : public limit_order_api_object {
                extended_limit_order() {
                }

                extended_limit_order(const limit_order_object &o)
                        : limit_order_api_object(o) {
                }

                double real_price = 0;
                bool rewarded = false;
            };




            /**
             *  Convert's vesting shares
             */
            struct extended_account : public account_api_object {
                extended_account() {
                }

                extended_account(const account_object &a, const golos::chain::database &db)
                        : account_api_object(a, db) {
                }

                asset vesting_balance; /// convert vesting_shares to vesting steem
                set<string> witness_votes;
            };


            struct candle_stick {
                time_point_sec open_time;
                uint32_t period = 0;
                double high = 0;
                double low = 0;
                double open = 0;
                double close = 0;
                double steem_volume = 0;
                double dollar_volume = 0;
            };

            struct order_history_item {
                time_point_sec time;
                string type; // buy or sell
                asset sbd_quantity;
                asset steem_quantity;
                double real_price = 0;
            };

            struct market {
                vector<extended_limit_order> bids;
                vector<extended_limit_order> asks;
                vector<order_history_item> history;
                vector<int> available_candlesticks;
                vector<int> available_zoom;
                int current_candlestick = 0;
                int current_zoom = 0;
                vector<candle_stick> price_history;
            };


        }
    }
}

FC_REFLECT_DERIVED((golos::plugins::database_api::extended_account),
                   ((golos::plugins::database_api::account_api_object)),
                   (vesting_balance)
                   (witness_votes)
)



//FC_REFLECT((golos::plugins::database_api::state), (current_route)(props)(category_idx)(tag_idx)(categories)(tags)(content)(accounts)(pow_queue)(witnesses)(discussion_idx)(witness_schedule)(feed_price)(error)(market_data))

FC_REFLECT_DERIVED((golos::plugins::database_api::extended_limit_order), ((golos::plugins::database_api::limit_order_api_object)), (real_price)(rewarded))
FC_REFLECT((golos::plugins::database_api::order_history_item), (time)(type)(sbd_quantity)(steem_quantity)(real_price));
FC_REFLECT((golos::plugins::database_api::market), (bids)(asks)(history)(price_history)(available_candlesticks)(available_zoom)(current_candlestick)(current_zoom))
FC_REFLECT((golos::plugins::database_api::candle_stick), (open_time)(period)(high)(low)(open)(close)(steem_volume)(dollar_volume));

#pragma once

#include <golos/chain/global_property_object.hpp>
#include <golos/chain/account_object.hpp>
#include <golos/chain/steem_objects.hpp>
#include <golos/chain/steem_object_types.hpp>
#include <golos/plugins/database_api/api_objects/account_api_object.hpp>
#include <golos/plugins/database_api/api_objects/witness_api_object.hpp>
#include "forward.hpp"
#include <golos/plugins/database_api/applied_operation.hpp>

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
                share_type reputation = 0;
                map<uint64_t, applied_operation> transfer_history; /// transfer to/from vesting
                map<uint64_t, applied_operation> market_history; /// limit order / cancel / fill
                map<uint64_t, applied_operation> post_history;
                map<uint64_t, applied_operation> vote_history;
                map<uint64_t, applied_operation> other_history;
                set<string> witness_votes;
                vector<pair<string, uint32_t>> tags_usage;
                vector<pair<account_name_type, uint32_t>> guest_bloggers;

                optional<map<uint32_t, extended_limit_order>> open_orders;
                optional<vector<string>> comments; /// permlinks for this user
                optional<vector<string>> blog; /// blog posts for this user
                optional<vector<string>> feed; /// feed posts for this user
                optional<vector<string>> recent_replies; /// blog posts for this user
                optional<vector<string>> recommended; /// posts recommened for this user
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
                   (vesting_balance)(reputation)
                           (transfer_history)(market_history)(post_history)(vote_history)(other_history)(witness_votes)(tags_usage)(guest_bloggers)(open_orders)(comments)(feed)(blog)(recent_replies)(recommended))



//FC_REFLECT((golos::plugins::database_api::state), (current_route)(props)(category_idx)(tag_idx)(categories)(tags)(content)(accounts)(pow_queue)(witnesses)(discussion_idx)(witness_schedule)(feed_price)(error)(market_data))

FC_REFLECT_DERIVED((golos::plugins::database_api::extended_limit_order), ((golos::plugins::database_api::limit_order_api_object)), (real_price)(rewarded))
FC_REFLECT((golos::plugins::database_api::order_history_item), (time)(type)(sbd_quantity)(steem_quantity)(real_price));
FC_REFLECT((golos::plugins::database_api::market), (bids)(asks)(history)(price_history)(available_candlesticks)(available_zoom)(current_candlestick)(current_zoom))
FC_REFLECT((golos::plugins::database_api::candle_stick), (open_time)(period)(high)(low)(open)(close)(steem_volume)(dollar_volume));
